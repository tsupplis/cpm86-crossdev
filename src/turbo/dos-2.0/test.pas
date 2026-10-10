{$C-}
program test;

var
  number,  sum, temp: real;
  count, index: integer;
  continue: boolean;
  answer: char;

function InvSquare(arg: real): real;
begin
 InvSquare := 1 / Sqr(arg)
end;

begin
  LowVideo;
  continue := true;
  while continue do
    begin
      write('Please tell me how many terms you want to add: ');
      readln(count);
      sum := 0;
      for index := 1 to count do
        begin
          number := index;
          sum := sum + InvSquare(number)
        end;
      HighVideo;
      writeln('1/1 + 1/4 +... + 1/',count,'*',count,' = ',sum);
      LowVideo;
      writeln;
      write('Do you wish to continue: ');
      readln(answer);
      if UpCase(answer) <> 'Y' then continue := false;
    end;
end.