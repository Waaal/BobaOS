
;Placehodler for jmp and nop
dw 0
db 0

; FAT32 header
OEMIdEntifier			db 'BOBAV0.1'
BytesPerSector			dw 0x200		; 512 bytes
SectorsPerCluser 		db 0x1			; 1 sectors per cluster
ReservedSectors			dw 32			; Store kernel in the reserved sectors
FATCopies 				db 0x02
RootDirEntries			dw 0x00			; 0 for fat32
NumSectors				dw 0x00
MediaType				db 0xF8
SectorsPerFat12_16		dw 0			; 0 for fat32
SectorsPerTrack			dw 0x20	
NumberOfHeads			dw 0x04
HiddenSectors			dd 0x00
SectorsBig 				dd 0x11000		; 34 MB

; FAT32 extended
SectorsPerFat32			dd 0x200		; (numClusters * 4) / sizeOfSector (512) (also minus reserved and actuall FAT because the FAT only holds data clusters but eh)
Flags					dw 0
FatVersion				dw 0
RootDirCluster			dd 2 			; Cluster 2 is always start of data cluster (hard coded in FAT standarts)
										; Start of data cluster (2): reserved 200 + (2*FAT = 1024) = 1224 = start of data cluster. (Next cluster = 1225,1226 etc)
FsInfoSector			dw 1
backUpBoot				dw 6			; We dont have one... but fuck it
times 12 db 0							; reserved
DriveNumber				db 0x80
WinNTBit				db 0x00
Signature 				db 0x29
VolumeID				dd 0xD106
VolumeIDString 			db 'BOBAOS BOOT'
SystemIDString 			db 'FAT32   '

times 512 - ($ - $$) db 0
