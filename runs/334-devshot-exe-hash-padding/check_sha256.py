import hashlib, os, pathlib, subprocess, tempfile
root = pathlib.Path.cwd()
exe = pathlib.Path(r'E:\Projects\f2r-291-evidence-images-leave-git\build\rule1-9f54f368\ci\fallout2-ce.exe')
old = subprocess.check_output(['git', 'show', 'facbdb671:ce/src/devtools/devtools.cc'], text=True)
new = (root / 'ce/src/devtools/devtools.cc').read_text(encoding='utf-8')
msvc = pathlib.Path(os.environ['ProgramFiles(x86)']) / r'Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.40.33807'
sdk = pathlib.Path(os.environ['ProgramFiles(x86)']) / r'Windows Kits\10'
v = '10.0.22621.0'
env = os.environ.copy()
env['PATH'] = str(msvc / 'bin/Hostx64/x64') + ';' + env['PATH']
env['INCLUDE'] = ';'.join(str(x) for x in [msvc/'include', sdk/'Include'/v/'ucrt', sdk/'Include'/v/'um', sdk/'Include'/v/'shared'])
env['LIB'] = ';'.join(str(x) for x in [msvc/'lib/x64', sdk/'Lib'/v/'ucrt/x64', sdk/'Lib'/v/'um/x64'])
with tempfile.TemporaryDirectory(prefix='f2r334-') as tmp:
    temp = pathlib.Path(tmp)
    for name, src in [('before', old), ('after', new)]:
        routine = src[src.index('struct Sha256 {'):src.index('// --- helpers', src.index('struct Sha256 {'))]
        code = '#include <cstdio>\n#include <cstring>\n' + routine + '\nint main(int argc, char** argv) { if (argc != 2) return 2; char hex[65]; sha256File(argv[1], hex, sizeof(hex)); puts(hex); }\n'
        cpp = temp / (name + '.cc'); cpp.write_text(code)
        subprocess.run([str(msvc / 'bin/Hostx64/x64/cl.exe'), '/nologo', '/O2', '/EHsc', str(cpp), '/Fe:' + str(temp / (name + '.exe'))], cwd=temp, env=env, check=True, stdout=subprocess.DEVNULL)
    for n in [0,1,55,56,57,58,59,60,61,62,63,64,119,120,121,127,128,184,1024]:
        data = bytes((i * 37 + 11) % 256 for i in range(n))
        f = temp / 'vector.bin'; f.write_bytes(data)
        expected = hashlib.sha256(data).hexdigest()
        actual = subprocess.check_output([str(temp/'after.exe'), str(f)], text=True).strip()
        assert actual == expected, (n, expected, actual)
        if 56 <= n <= 63:
            before = subprocess.check_output([str(temp/'before.exe'), str(f)], text=True).strip()
            assert before != expected, n
    expected = hashlib.file_digest(exe.open('rb'), 'sha256').hexdigest()
    before = subprocess.check_output([str(temp/'before.exe'), str(exe)], text=True).strip()
    after = subprocess.check_output([str(temp/'after.exe'), str(exe)], text=True).strip()
    assert expected == '87a6e21031a9c3825b6fd99ecdd7498cf6ac92c1ad5e19aeb670e88dae5d8a0f'
    assert before == 'b5d667ebe0812c27a9d37978fab57ad3ca4cb198b734815cad51e2f311f50cd9'
    assert after == expected
    print('PASS vectors: 0, 1, 55-64, 119-121, 127, 128, 184, 1024; before fails 56-63')
    print('CI executable bytes:', exe.stat().st_size, 'before:', before, 'after/stdlib:', after)


