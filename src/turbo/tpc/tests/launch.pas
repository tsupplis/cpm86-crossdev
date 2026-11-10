program launch;
{$I shared.inc}
begin
  SharedValue := 1234;
  assign(OutputFile, 'before.txt');
  rewrite(OutputFile);
  writeln(OutputFile, 'BEFORE=', SharedValue:1);
  close(OutputFile);
  assign(ChainFile, 'chain.chn');
  chain(ChainFile);
  assign(OutputFile, 'return.txt');
  rewrite(OutputFile);
  writeln(OutputFile, 'FAIL: Chain returned');
  close(OutputFile)
end.
