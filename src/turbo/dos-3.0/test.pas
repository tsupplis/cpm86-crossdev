program test;

overlay procedure o;
begin
     writeln('o');
end;

var
   l: string[255];
   f: text;
   n: string[14];
   a: array[1..1024] of byte;
begin
     clrscr;
     textcolor(3);
     write('File:');
     read(n);
     assign(f,n);
     reset(f);
     while not eof(f) do begin
           readln(f,l);
           writeln(l);
     end;
     o;
     close(f);
end.