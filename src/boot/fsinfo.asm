[org 0x7E00]
[bits 16]

;Some extended info needed for the fat32 file system
FSInfo:
dd 0x41615252	; lead signature
times 480 db 0  ; reserved
dd 0x61417272	; struct signature
dd 0xFFFFFFFF	; free cluster count (0xFFFFFFFF = unknown)
dd 0x00000002	; next free cluster
times 12 db 0
dd 0xAA550000	; Trail signature
