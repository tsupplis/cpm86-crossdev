program fibonacci;

function fib(n:integer): integer;
begin
    if (n <= 2) then
        fib := 1
    else
        fib := fib(n-1) + fib(n-2);
end;

var
    i:integer;

begin
    for i := 1 to 16 do
        write(fib(i), ', ');
    writeln('...');
end.
