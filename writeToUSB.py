import os

# read boot.iso
file_fd = os.open(r"boot.bin", os.O_RDONLY | os.O_BINARY)
data = os.read(file_fd, os.stat(r"boot.bin").st_size)
os.close(file_fd)

#write to physicaldrive5
usb_fd = os.open(r"\\.\PhysicalDrive5", os.O_WRONLY | os.O_BINARY)
os.write(usb_fd, data)
os.close(usb_fd)
