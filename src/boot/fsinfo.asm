;Some extended info needed for the fat32 file system
FSInfo:
dd 0x41615252	; lead signature
times 480 db 0  ; reserved
dd 0x61417272	; struct signature
dd 0x10BDF		; free cluster count (1 sector = 1 cluster)(we have a total of 69632 clusters. - 2*FAT(512 * 2) - 32 reserved sectors. and -1 cause -1 root dir cluster)
dd 0x00000002	; next free cluster
times 510 - ($ - $$) db 0
db 0x55
db 0xAA
