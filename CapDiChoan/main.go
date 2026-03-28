//서버: 받은 메시지에 대해 120초 뒤에 지우는방식.
//만약 클라이언트를 짠다면, sulmyeong.md의 사진처럼 트랜잭션을 서버-클라이언트 양쪽 아다리를 맞춰야할듯.. 어캐해야하려나..

package main

import (
	"bytes"
	"errors"
	"fmt"
	"io"
	"log"
	"net/http"
	"strings"
	"sync"
	"time"
)

const (
	addr       = ":8080"
	ttl        = 120 * time.Second
	maxBodyLen = 64 << 10 // 64 KiB
)

type message struct {
	Body      string
	ExpiresAt time.Time
}

type board struct {
	mu           sync.Mutex
	messages     []message
	snapshot     []byte
	cleanupTimer *time.Timer // 전역 타이머 1개
}

func newBoard() *board {
	return &board{
		snapshot: make([]byte, 0),
	}
}

func normalizeNewlines(s string) string {
	s = strings.ReplaceAll(s, "\r\n", "\n")
	s = strings.ReplaceAll(s, "\r", "\n")
	return s
}

func (b *board) purgeExpiredLocked(now time.Time) {
	n := 0
	for n < len(b.messages) && !b.messages[n].ExpiresAt.After(now) {
		n++
	}
	if n == 0 {
		return
	}

	copy(b.messages, b.messages[n:])
	newLen := len(b.messages) - n
	for i := newLen; i < len(b.messages); i++ {
		b.messages[i] = message{}
	}
	b.messages = b.messages[:newLen]
}

func (b *board) rebuildSnapshotLocked() {
	if len(b.messages) == 0 {
		b.snapshot = b.snapshot[:0]
		return
	}

	var buf bytes.Buffer

	// 최신 메시지가 먼저 보이도록 역순 출력
	for i := len(b.messages) - 1; i >= 0; i-- {
		m := b.messages[i]

		// plain text framing:
		// Expires-At-Unix: <unix>
		// Content-Length: <body-bytes>
		//
		// <raw body>
		//
		fmt.Fprintf(&buf, "Expires-At-Unix: %d\n", m.ExpiresAt.Unix())
		fmt.Fprintf(&buf, "Content-Length: %d\n\n", len(m.Body))
		buf.WriteString(m.Body)
		buf.WriteString("\n\n")
	}

	b.snapshot = append(b.snapshot[:0], buf.Bytes()...)
}

func (b *board) scheduleNextCleanupLocked() {
	if len(b.messages) == 0 {
		b.cleanupTimer = nil
		return
	}

	// 오래된 메시지부터 죽음
	d := time.Until(b.messages[0].ExpiresAt)
	if d < 0 {
		d = 0
	}

	b.cleanupTimer = time.AfterFunc(d, b.cleanupCallback)
}

func (b *board) cleanupCallback() {
	b.mu.Lock()
	defer b.mu.Unlock()

	// 이번 타이머는 소모 완료
	b.cleanupTimer = nil

	now := time.Now()
	b.purgeExpiredLocked(now)
	b.rebuildSnapshotLocked()

	// 남아 있으면 다음 만료 시각 기준으로 다시 타이머 1개만 예약
	if len(b.messages) > 0 {
		b.scheduleNextCleanupLocked()
	}
}

func (b *board) push(body string, now time.Time) time.Time {
	expiresAt := now.Add(ttl)

	b.mu.Lock()
	defer b.mu.Unlock()

	// 혹시 아직 남아 있던 만료분이 있으면 정리
	b.purgeExpiredLocked(now)

	b.messages = append(b.messages, message{
		Body:      body,
		ExpiresAt: expiresAt,
	})
	b.rebuildSnapshotLocked()

	// TTL이 고정이므로, 이미 타이머가 있다면 가장 오래된 메시지 기준 예약은 그대로 맞다.
	// 큐가 비어 있던 상태에서 첫 메시지가 들어온 경우에만 새로 단다.
	if b.cleanupTimer == nil {
		b.scheduleNextCleanupLocked()
	}

	return expiresAt
}

func (b *board) currentSnapshot() []byte {
	b.mu.Lock()
	defer b.mu.Unlock()

	out := make([]byte, len(b.snapshot))
	copy(out, b.snapshot)
	return out
}

func noStoreHeaders(w http.ResponseWriter) {
	w.Header().Set("Cache-Control", "no-store")
	w.Header().Set("Pragma", "no-cache")
	w.Header().Set("Expires", "0")
}

func main() {
	brd := newBoard()
	mux := http.NewServeMux()

	mux.HandleFunc("/push", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			w.Header().Set("Allow", http.MethodPost)
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}

		r.Body = http.MaxBytesReader(w, r.Body, maxBodyLen)
		defer r.Body.Close()

		raw, err := io.ReadAll(r.Body)
		if err != nil {
			var maxErr *http.MaxBytesError
			if errors.As(err, &maxErr) {
				http.Error(w, "message too large", http.StatusRequestEntityTooLarge)
				return
			}
			http.Error(w, "failed to read body", http.StatusBadRequest)
			return
		}

		body := normalizeNewlines(string(raw))
		if len(body) == 0 {
			http.Error(w, "empty body", http.StatusBadRequest)
			return
		}

		expiresAt := brd.push(body, time.Now())

		noStoreHeaders(w)
		w.Header().Set("Content-Type", "text/plain; charset=utf-8")
		w.WriteHeader(http.StatusCreated)
		_, _ = fmt.Fprintf(w, "ok\nexpires-at-unix=%d\n", expiresAt.Unix())
	})

	mux.HandleFunc("/poll", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			w.Header().Set("Allow", http.MethodGet)
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}

		out := brd.currentSnapshot()

		noStoreHeaders(w)
		w.Header().Set("Content-Type", "text/plain; charset=utf-8")
		w.WriteHeader(http.StatusOK)
		_, _ = w.Write(out)
	})

	srv := &http.Server{
		Addr:              addr,
		Handler:           mux,
		ReadHeaderTimeout: 5 * time.Second,
		ReadTimeout:       10 * time.Second,
		WriteTimeout:      10 * time.Second,
		IdleTimeout:       30 * time.Second,
	}

	log.Printf("listening on %s", addr)
	log.Fatal(srv.ListenAndServe())
}
