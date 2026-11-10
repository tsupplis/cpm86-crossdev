program comtest;
{$I values.inc}
type
  numbers = array[1..32] of integer;
  NumbersPtr = ^numbers;
var
  p: NumbersPtr;
  f: text;
  i, total, InputValue: integer;
  value: real;

function SumSquares: integer;
var
  n, sum: integer;
begin
  sum := 0;
  for n := 1 to 32 do
    sum := sum + p^[n];
  SumSquares := sum
end;

begin
  assign(f, 'input.txt');
  reset(f);
  readln(f, InputValue);
  close(f);
  new(p);
  for i := 1 to 32 do
    p^[i] := i * i;
  total := SumSquares;
  dispose(p);
  value := sqrt(81.0) + sin(0.0) + ln(exp(1.0));
  assign(f, 'comtest.txt');
  rewrite(f);
  if (total = ExpectedSum) and (round(value) = 10) then
    writeln(f, 'COM=', total + InputValue:1, ';REAL=', round(value):1)
  else
    writeln(f, 'FAIL');
  close(f)
end.
