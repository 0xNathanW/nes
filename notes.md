For testing:

Set your program counter to 0xC000 and run that ROM. 
Set up some way to print out each instruction as you execute it, and compare that to the nestest.log file. 
nestest.nes is a pretty comprehensive test of all of the normal instructions and the stable undocumented ones. 

## Mappers
Hardware chips that control how ROM is accessed.
Larger games needed more ROM space than the original 64KB.
Mappers solve this with bank switching.  By writing to certain addresses
the mapper can dynamically change which portions of ROM are visible to the CPU.

Simple:
NROM (Mapper 0):
- no banking
- 16KB or 32KB prg rom
- 8KB chr rom

CNROM (Mapper 3):
- chr banking only (grpahics)
- 16KB or 32KB prg rom

Medium (TODO):
- UxROM
- AOROM

Hard (TODO):
- MMC1
- MMC3