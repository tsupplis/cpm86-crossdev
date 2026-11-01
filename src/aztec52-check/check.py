#!/usr/bin/env python3
"""Investigate 5.2 tools with installed patched 4.2 libraries; never install tools.
Usage: python3 check.py /path/to/az8652a.zip /path/to/new-work-directory
"""
from pathlib import Path
import json
import os
import shutil
import subprocess
import sys
import zipfile

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
archive=Path(sys.argv[1]).resolve()
work=Path(sys.argv[2]).resolve()
work.mkdir(parents=True,exist_ok=False)
toolroot=work/'tools'
with zipfile.ZipFile(archive) as z:
    for entry in z.infolist():
        destination=toolroot/entry.filename.lower()
        if not destination.resolve().is_relative_to(toolroot):
            raise ValueError('Unsafe archive entry')
        if entry.is_dir():destination.mkdir(parents=True,exist_ok=True)
        else:
            destination.parent.mkdir(parents=True,exist_ok=True)
            destination.write_bytes(z.read(entry))
tools=toolroot/'az8652a'
(work/'temp').mkdir()
for name in ('abi.h','provider.c','consumer.c','limits.c','constants.c'):
    shutil.copyfile(HERE/name,work/name)
for name in ('regress.c','inf.c','truncate.c'):
    shutil.copyfile(ROOT/'src/aztecreg'/name,work/name)
source=(work/'regress.c').read_text()
index=source.index('#define CHECK')
source=source[:index]+'''/* Investigation-only exact 4.2 overflow sentinel; not an installed fix. */
static union { unsigned short w[4]; double value; } compat_huge = {{0xbb99,0x58ad,0xdcf1,0x7fef}};
#undef HUGE_VAL
#define HUGE_VAL compat_huge.value
'''+source[index:]
(work/'regfix.c').write_text(source)
bin=ROOT/'bin'
env=os.environ.copy()
env.update(EMU2_DRIVE_D=str(tools),EMU2_DRIVE_E=str(work/'temp'),
           EMU2_DRIVE_F=str(ROOT/'share/aztec42'),EMU2_PROGNAME='D:\\BIN\\CC.EXE')
results={}

def command(args,log,environment=None):
    # PIPE is essential: truncate tests reject stdout only when it is a device.
    p=subprocess.run([str(a) for a in args],cwd=work,env=environment,
                     stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=120)
    log.append(p.stdout.decode(errors='replace'))
    if p.returncode:raise RuntimeError('exit '+str(p.returncode))
    return log[-1]

def compile(name,obj,model,log,new=True,extra=()):
    if new:
        args=[bin/'emu2',tools/'bin/cc.exe','+'+model,'+F','+0',*extra,'-o',obj,name,
              '--','PATH=D:\\BIN','INCLUDE=F:\\INCLUDE','CLIB=F:\\LIB\\','CCTEMP=E:\\']
        return command(args,log,env)
    return command([bin/'aztec42_cc','+'+model,'+F','+0',*extra,'-o',obj,name],log)

def record(key,action):
    log=[]
    try:
        action(log)
        results[key]='PASS'
    except Exception as e:
        results[key]='FAIL: '+str(e)
    (work/(key+'.log')).write_text(''.join(log))
    print(key,results[key],flush=True)

for target in ('c86','d11','c','clc','cld','cl'):
    model={'clc':'lc','cld':'ld','cl':'l'}.get(target,'')
    output='run.'+('cmd' if target=='c86' else 'com' if target=='d11' else 'exe')
    runner=bin/('cpm86' if target=='c86' else 'emu2')
    if target=='d11' and os.environ.get('AZ52_DOS_EMULATOR'):
        runner=Path(os.environ['AZ52_DOS_EMULATOR'])
    flags=() if target=='c86' else ('-DDOS_TIME',)
    def execute(log,objects,marker):
        command([bin/'aztec42_link','-o',output,*objects,'-lm'+model,'-l'+target],log)
        text=command([runner,output],log)
        if marker not in text or 'FAIL' in text:raise RuntimeError('guest assertions')
    for test,marker in (('regress','AZTEC PASS'),('regfix','AZTEC PASS'),
                        ('inf','INF CORE PASS'),('truncate','TRUNCATE PASS')):
        if target=='c86' and test=='truncate':continue
        def test_action(log):
            compile(test+'.c','test.o',model,log,extra=flags)
            execute(log,['test.o'],marker)
        record(target+'-'+test,test_action)
    for newcaller in (True,False):
        def abi_action(log):
            compile('consumer.c','consumer.o',model,log,new=newcaller)
            compile('provider.c','provider.o',model,log,new=not newcaller)
            execute(log,['consumer.o','provider.o'],'ABI PASS')
        record(target+('-newcaller' if newcaller else '-oldcaller'),abi_action)

for new in (False,True):
    def limits_action(log):
        compile('limits.c','limits.o','',log,new=new)
        command([bin/'aztec42_link','-o','limits.exe','limits.o','-lm','-lc'],log)
        command([bin/'emu2','limits.exe'],log)
    record('limits-'+('52' if new else '42'),limits_action)
for new in (False,True):
    def constants_action(log):
        compile('constants.c','constants.o','',log,new=new)
        command([bin/'aztec42_link','-o','constants.exe','constants.o','-lc'],log)
        command([bin/'emu2','constants.exe'],log)
    record('constants-'+('52' if new else '42'),constants_action)

expr=ROOT.parent/'cpm86-ports/expr'
if expr.is_dir():
    for path in list(expr.glob('*.c'))+list(expr.glob('*.h')):
        shutil.copyfile(path,work/path.name)
    def expr_action(log):
        compile('smoke.c','smoke.o','',log,extra=('-I.','-DREQUIRE_INFINITY'))
        compile('tinyexpr.c','tinyexpr.o','',log,extra=('-I.',))
        command([bin/'aztec42_link','-o','smoke.cmd','smoke.o','tinyexpr.o','-lm','-lc86'],log)
        text=command([bin/'cpm86','smoke.cmd'],log)
        if 'ALL TESTS PASSED' not in text or 'skipped' in text.lower():
            raise RuntimeError('expr assertions')
    record('expr-c86',expr_action)

(work/'results.json').write_text(json.dumps(results,indent=2)+'\n')
print('Raw regressions intentionally expose the HUGE_VAL mismatch; inspect logs.')
