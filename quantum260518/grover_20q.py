#source .venv/bin/activate 후 여기에 적당한 qiskit_aer, matplotlib 필요.,
import math
from qiskit import QuantumCircuit, transpile
from qiskit_aer import AerSimulator

N_QUBITS = 20 #aer 환경에서 1큐비트 늘때마다 연산이 먹는 메모리가 2배 는다고..
TARGET = "10101110101101101101" #20비트의 '임의의 숫자'
SHOTS = 16384

def validate_target(n_qubits:int, target: str) -> None:
    if len(target) != n_qubits:
        raise ValueError("TARGET length must match N_QUBIT")
    if any(bit not in "01" for bit in target):
        raise ValueError("TARGET bits only supports 0 or 1")

#아다마르 연산을 보조하는 함수임. 여기서 핵심은 양자 큐비트 계산 무조건 뒤비트부터
def bit_for_qubit(target: str, qubit_index: int) -> str: 
    return target[::-1][qubit_index]

#grover에서 '모든 큐비트가 1로 맞춰지는 상황(참인 상황)에만 위상 뒤집기 작업. 
#아다마르 게이트 2번은 원래대로 돌아오는데 아다마르->위상뒤집->아다마르 순서 진행
def apply_multi_controlled_z(circuit: QuantumCircuit, n_qubits: int) -> None:
    if n_qubits == 1:
        circuit.z(0)
        return
    controls = list(range(n_qubits - 1))
    z_target = n_qubits - 1
    circuit.h(z_target)
    circuit.mcx(controls, z_target)
    circuit.h(z_target)

#target 상태일때만 apply_multi_controlled_z 함수 호출해 위상 뒤집음.
def apply_phase_oracle(circuit: QuantumCircuit, target: str) -> None:
    n_qubits = len(target)
    for q in range(n_qubits):
        if bit_for_qubit(target, q) == "0":
            circuit.x(q)
    apply_multi_controlled_z(circuit, n_qubits)
    for q in range(n_qubits):
        if bit_for_qubit(target, q) == "0":
            circuit.x(q)

#확률의 합은 항상 1로 유지해야되기에 diffuse 필요(오답에 해당하는 값에 destructive amplitude)
#ibm 피셜 diffuser을 만들 때 이렇게 만들어라. phase차이를 amplitude차이로 변환
def apply_diffuser(circuit: QuantumCircuit, n_qubits: int) -> None:
    circuit.h(range(n_qubits))
    circuit.x(range(n_qubits))
    apply_multi_controlled_z(circuit, n_qubits)
    circuit.x(range(n_qubits))
    circuit.h(range(n_qubits))

#n비트 그로버는 수학적으로 시행이 ㅠ * 2^n/2 / 4 - 1/2이 100퍼확률로 이상적 해가 나오는 지점.
def optimal_iterations(n_qubits: int, marked_count: int = 1) -> int:
    search_space = 2 ** n_qubits
    theta = math.asin(math.sqrt(marked_count / search_space))
    return round((math.pi / (4 * theta)) - 0.5)

def build_grover_circuit(n_qubits: int, target: str) -> tuple[QuantumCircuit, int]:
    validate_target(n_qubits, target)
    iterations = optimal_iterations(n_qubits, marked_count=1) 
    circuit = QuantumCircuit(n_qubits, n_qubits)
    circuit.h(range(n_qubits))
    for _ in range(iterations):
        apply_phase_oracle(circuit, target)
        apply_diffuser(circuit, n_qubits)
    circuit.measure(range(n_qubits), range(n_qubits))
    return circuit, iterations


circuit, iterations = build_grover_circuit(N_QUBITS, TARGET)
#aer 상태벡터 시뮬레이터 생성(만약 클라우드 qpu활용할거면 여기 수정 필요)
backend = AerSimulator(method="statevector")

#여기부터 main
try: 
    result = backend.run(circuit, shots=SHOTS).result() 
except Exception as exc:
    print(f"Direct Aer run failed.... so transpiling...: {exc}")
    compiled = transplie(circuit, backend, optimization_level=0)
    result = backend.run(compiled, shots=SHOTS).result()

counts = result.get_counts()
best = max(counts, key=counts.get)

theta = math.asin(1 / math.sqrt(2 ** N_QUBITS))
expected_success = math.sin((2 * iterations + 1) * theta) ** 2
top10 = sorted(counts.items(), key=lambda item: item[1], reverse=True)[:10]

print(f"qubits              : {N_QUBITS}")                              
print(f"target              : {TARGET}")                                
print(f"Grover iterations   : {iterations}")                            
print(f"theoretical success : {expected_success:.10f}")                 
print(f"best measured value : {best}")                                 
print(f"target hits         : {counts.get(TARGET, 0)} / {SHOTS}")       
print(f"top-10 counts       : {top10}") # 노이즈때문에 생기는 이상값들 옆에 전시됨.            

#코드를 직접 손으로 짜면서 느낀게, 이론 볼 때 2게이트 entanglement 유도하는 게이트들이 초고비용 게이트라 들었는데 이거 줄이는게 엄청엄청어렵구나.. 시픙ㅁ
