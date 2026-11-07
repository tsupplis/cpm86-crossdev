codeseg segment para public 'code'
dataseg segment para public 'data'
 public failclose_
failclose_ dw 0
dataseg ends
 assume cs:codeseg,ds:dataseg
 public bdos_
bdos_ proc near
 push bp
 mov bp,sp
 push di
 push si
 mov cx,4[bp]
 mov dx,6[bp]
 int 224
 cmp word ptr 4[bp],16
 jne normal
 cmp failclose_,0
 je normal
 mov failclose_,0
 mov ax,255
normal:
 and ax,255
 pop si
 pop di
 pop bp
 ret
bdos_ endp
codeseg ends
 end
