const https = require('https');
const fs = require('fs');
const path = require('path');
const os = require('os');
const { execSync } = require('child_process');

const VERSION = require('./package.json').version;
const REPO = 'Rampyaaryans/rampyaaryan';

const PLATFORM_MAP = {
    'win32':  { asset: 'Rampyaaryan-Windows-Portable.zip', binary: 'rampyaaryan.exe' },
    'linux':  { asset: 'Rampyaaryan-Linux-Portable.tar.gz', binary: 'rampyaaryan' },
    'darwin': { asset: 'Rampyaaryan-macOS-Portable.tar.gz', binary: 'rampyaaryan' }
};

const platform = os.platform();
const info = PLATFORM_MAP[platform];

if (!info) {
    console.error(`Platform "${platform}" is not supported.`);
    process.exit(1);
}

const binDir = path.join(__dirname, 'bin');
const binaryPath = path.join(binDir, info.binary);

if (fs.existsSync(binaryPath)) {
    console.log('Rampyaaryan binary already exists. Skipping download.');
    process.exit(0);
}

const url = `https://github.com/${REPO}/releases/latest/download/${info.asset}`;
const tmpFile = path.join(os.tmpdir(), info.asset);

console.log(`Downloading Rampyaaryan for ${platform}...`);
console.log(`  ${url}`);

function download(url, dest) {
    return new Promise((resolve, reject) => {
        const file = fs.createWriteStream(dest);
        https.get(url, (response) => {
            if (response.statusCode === 302 || response.statusCode === 301) {
                download(response.headers.location, dest).then(resolve).catch(reject);
                return;
            }
            if (response.statusCode !== 200) {
                reject(new Error(`Download failed: HTTP ${response.statusCode}`));
                return;
            }
            response.pipe(file);
            file.on('finish', () => { file.close(); resolve(); });
        }).on('error', reject);
    });
}

async function install() {
    try {
        await download(url, tmpFile);
        fs.mkdirSync(binDir, { recursive: true });

        if (platform === 'win32') {
            // Use PowerShell to extract zip
            execSync(`powershell -Command "Expand-Archive -Path '${tmpFile}' -DestinationPath '${binDir}' -Force"`, { stdio: 'inherit' });
        } else {
            execSync(`tar xzf "${tmpFile}" -C "${binDir}"`, { stdio: 'inherit' });
            fs.chmodSync(binaryPath, 0o755);
        }

        console.log('Rampyaaryan installed successfully!');
        console.log('Run: rampyaaryan --help');

        // Cleanup
        try { fs.unlinkSync(tmpFile); } catch (_) {}
    } catch (err) {
        console.error('Installation failed:', err.message);
        console.error('You can download manually from:');
        console.error(`  https://github.com/${REPO}/releases/latest`);
        process.exit(1);
    }
}

install();
