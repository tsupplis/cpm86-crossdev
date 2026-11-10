program ChainTest;
{$I shared.inc}
begin
  assign(OutputFile, 'chain.txt');
  rewrite(OutputFile);
  if SharedValue = 1234 then
  begin
    SharedValue := SharedValue + 42;
    writeln(OutputFile, 'CHAIN=', SharedValue:1)
  end
  else
    writeln(OutputFile, 'FAIL: shared data lost');
  close(OutputFile)
end.
