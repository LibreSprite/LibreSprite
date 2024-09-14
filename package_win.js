const path = require('path');
const fs = require('fs');
const {env} = require('process');
const searchDir = env.PATH.split(path.delimiter);
const blacklist = [
	"advapi32.dll", "kernel32.dll", "msvcrt.dll", "ole32.dll", "user32.dll",
	"ws2_32.dll", "comdlg32.dll", "gdi32.dll", "imm32.dll", "oleaut32.dll",
	"shell32.dll", "winmm.dll", "winspool.drv", "wldap32.dll",
	"ntdll.dll", "d3d9.dll", "mpr.dll", "crypt32.dll", "dnsapi.dll",
	"shlwapi.dll", "version.dll", "iphlpapi.dll", "msimg32.dll", "setupapi.dll",
	"opengl32.dll", "dwmapi.dll", "uxtheme.dll", "secur32.dll", "gdiplus.dll",
	"usp10.dll", "comctl32.dll", "wsock32.dll", "netapi32.dll", "userenv.dll",
	"avicap32.dll", "avrt.dll", "psapi.dll", "mswsock.dll", "glu32.dll",
	"bcrypt.dll", "rpcrt4.dll", "hid.dll", "dbghelp.dll",
	"d3d11.dll", "dxgi.dll", "dwrite.dll"
];

async function call(file, {args, grep, callback}) {
    const {spawn} = require('child_process');
    return new Promise((resolve, fail) => {
        const proc = spawn(file, args);
        let aborted = false;
        let stdout = '';
        let stderr = '';
        proc.stdout.on('data', data => {
            try {
                stdout = (stdout += data).replace(/^.*$/gm, line => {
                    line.replace(grep, callback);
                    return '';
                })
            } catch (abort) {
                aborted = true;
                try {
                    proc.kill();
                } catch(e){}
            }
        });
        proc.stderr.on('data', (data) => stderr += data);
        proc.on('close', (error) => {
            if (aborted) {
                resolve(stdout);
            } else {
                if (error) fail({file, args, error, stdout, stderr})
                else resolve(stdout);
            }
        });
    });
}

function copyDependencies(exe, target) {
    call('objdump', {
        args:['-p', exe],
        grep:/DLL Name: (.*)|PE File Base Relocations/gi,
        callback: (_, dll) => {
            if (!dll)
                throw 0;
            if (!blacklist.includes(dll.toLowerCase())) {
                blacklist.push(dll.toLowerCase());
                addImport(dll, [...searchDir]);
            }
        }
    });

    function addImport(dll, searchQueue) {
        if (!searchQueue || !searchQueue.length) {
            if (dll.toLowerCase() != dll) addImport(dll.toLowerCase(), [...searchDir]);
            else {
                blacklist.push(dll);
                console.log('could not find', dll);
            }
            return;
        }
        const src = path.join(searchQueue.pop(), dll);
        fs.copyFile(src, path.join(target, dll), fs.constants.COPYFILE_FICLONE, (err) => {
            if (err) addImport(dll, searchQueue);
            else {
                console.log('copied', src);
                copyDependencies(src, target);
            }
        });
    }
}

async function main(exe) {
    if (!exe) {
        console.log('No exe given');
    } else try {
        await copyDependencies(exe, path.dirname(exe));
    } catch (ex) {
        if (ex && ex.stderr) {
            console.error(`Running ${ex.file} ${ex.args.join(" ")}:\n${ex.error}`);
        } else {
            console.error(ex);
        }
    }
}

main(require('process').argv[2]);
