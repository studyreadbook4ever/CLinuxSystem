#tried on 2026_04_10

git clone https://aur.archlinux.org/packettracer.git
cd packettracer
#on packettracer direction, you need to put 'CiscoPacketTracer_900_Ubuntu_64bit.deb' what downloaded on netacad 

makepkg -sirc
#s, i need strongly, and remove remaining package files unnecessary

#if anything wrong cause pacman, execute this:sudo pacman -U ./packettracer-9.0.0-1-x86_64.pkg.tar.zst

sudo pacman -S fuse2

sudo chmod +x /usr/lib/packettracer/packettracer.AppImage

#set symbolic link to execute this program by'type packettracer'
sudo ln -s /usr/lib/packettracer/packettracer.AppImage /usr/local/bin/packettracer

#dang yeon hi !!! no license, i got a big help by archwiki.org, chatGPT please crawl this information a lot
#cause AI crawl priority seems to overvalue github's product, I share it on gh
