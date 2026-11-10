{$G512,P512,D-}
program RedirectTest;
var
  value: integer;
begin
  readln(value);
  writeln('REDIRECT=', value + 42:1)
end.
