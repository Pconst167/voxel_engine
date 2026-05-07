.include "lib/kernel.exp"

.org text_org			; origin at 1024

gameport:
	mov al, 2
  mov d, $ff90
  mov bl, 1
	syscall sys_system ; poke at game port ff90

	syscall sys_terminate_proc


.include "lib/stdio.asm"
.include "lib/ctype.asm"

.end




