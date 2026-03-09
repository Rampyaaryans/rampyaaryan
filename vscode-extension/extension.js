const vscode = require('vscode');
const path = require('path');
const fs = require('fs');
const { execFile } = require('child_process');

// ════════════════════════════════════════════════════════════════
//   Neko Cat  ·  Hindi Status Bar  ·  Mantra Mode  ·  Run .ram
//   Rampyaaryan VS Code Extension
// ════════════════════════════════════════════════════════════════

const HINDI_DIGITS = ['\u0966','\u0967','\u0968','\u0969','\u096A','\u096B','\u096C','\u096D','\u096E','\u096F'];

const MANTRAS = [
    '\u0950 \u0928\u092E\u0903 \u0936\u093F\u0935\u093E\u092F',
    '\u091C\u092F \u0936\u094D\u0930\u0940 \u0930\u093E\u092E',
    '\u0939\u0930\u0947 \u0915\u0943\u0937\u094D\u0923 \u0939\u0930\u0947 \u0930\u093E\u092E',
    '\u0938\u0924\u094D\u092F\u092E\u0947\u0935 \u091C\u092F\u0924\u0947',
    '\u0935\u0938\u0941\u0927\u0948\u0935 \u0915\u0941\u091F\u0941\u092E\u094D\u092C\u0915\u092E\u094D',
    '\u0915\u0930\u094D\u092E\u0923\u094D\u092F\u0947\u0935\u093E\u0927\u093F\u0915\u093E\u0930\u0938\u094D\u0924\u0947',
    '\u0950 \u091C\u092F \u091C\u0917\u0926\u0940\u0936 \u0939\u0930\u0947',
    '\u0936\u094D\u0930\u0940 \u0930\u093E\u092E \u091C\u092F \u0930\u093E\u092E \u091C\u092F \u091C\u092F \u0930\u093E\u092E',
    '\u0950 \u0936\u093E\u0902\u0924\u093F \u0936\u093E\u0902\u0924\u093F \u0936\u093E\u0902\u0924\u093F',
    '\u0938\u0930\u094D\u0935\u0947 \u092D\u0935\u0928\u094D\u0924\u0941 \u0938\u0941\u0916\u093F\u0928\u0903',
    '\u0924\u092E\u0938\u094B \u092E\u093E \u091C\u094D\u092F\u094B\u0924\u093F\u0930\u094D\u0917\u092E\u092F',
    '\u092F\u094B\u0917\u0903 \u0915\u0930\u094D\u092E\u0938\u0941 \u0915\u094C\u0936\u0932\u092E\u094D'
];

const RAM_QUOTES = [
    '\u0927\u0930\u094D\u092E \u0915\u0940 \u091C\u092F \u0939\u094B',
    '\u0930\u093E\u092E \u0928\u093E\u092E \u0938\u0924\u094D\u092F \u0939\u0948',
    '\u092C\u094B\u0932\u094B \u0938\u093F\u092F\u093E\u092A\u0924\u093F \u0930\u093E\u092E\u091A\u0928\u094D\u0926\u094D\u0930 \u0915\u0940 \u091C\u092F',
    '\u0930\u0918\u0941\u092A\u0924\u093F \u0930\u093E\u0918\u0935 \u0930\u093E\u091C\u093E \u0930\u093E\u092E',
    '\u092E\u0930\u094D\u092F\u093E\u0926\u093E \u092A\u0941\u0930\u0941\u0937\u094B\u0924\u094D\u0924\u092E',
    '\u0938\u0924\u094D\u092F \u0915\u093E \u092E\u093E\u0930\u094D\u0917 \u0939\u0940 \u0927\u0930\u094D\u092E \u0939\u0948',
    '\u091C\u092F \u0938\u093F\u092F\u093E\u0930\u093E\u092E',
    '\u092E\u0902\u0917\u0932 \u092D\u0935\u0928 \u0905\u092E\u0902\u0917\u0932 \u0939\u093E\u0930\u0940'
];

// ═══════ State ═══════
let hindiBarEnabled = true;
let mantraEnabled = false;

let hindiStatusItem = null;
let mantraStatusItem = null;
let ramQuoteItem = null;

let mantraInterval = null;
let mantraIndex = 0;
let quoteInterval = null;
let quoteIndex = 0;

// ═══════ Helpers ═══════

function toHindiNumerals(n) {
    return String(n).split('').map(d => HINDI_DIGITS[parseInt(d)] || d).join('');
}

// ═══════ Neko WebviewView (Panel area) ═══════

class NekoViewProvider {
    constructor(extensionUri) {
        this._extensionUri = extensionUri;
        this._view = null;
    }

    resolveWebviewView(webviewView) {
        this._view = webviewView;
        webviewView.webview.options = { enableScripts: true };
        const nekoHtmlPath = path.join(this._extensionUri.fsPath, 'media', 'neko.html');
        webviewView.webview.html = fs.readFileSync(nekoHtmlPath, 'utf8');
    }
}

let nekoViewProvider = null;

// ═══════ Hindi Status Bar ═══════

function updateHindiStatusBar(editor) {
    if (!hindiBarEnabled || !hindiStatusItem || !editor) {
        if (hindiStatusItem) hindiStatusItem.hide();
        return;
    }
    const pos = editor.selection.active;
    const line = toHindiNumerals(pos.line + 1);
    const col = toHindiNumerals(pos.character + 1);
    hindiStatusItem.text = `$(book) \u092A\u0902\u0915\u094D\u0924\u093F ${line} \u00b7 \u0938\u094D\u0924\u0902\u092D ${col}`;
    hindiStatusItem.tooltip = `Line ${pos.line + 1}, Column ${pos.character + 1}`;
    hindiStatusItem.show();
}

// ═══════ Mantra Mode ═══════

function startMantra() {
    mantraEnabled = true;
    if (mantraStatusItem) {
        mantraStatusItem.text = `$(heart) ${MANTRAS[mantraIndex]}`;
        mantraStatusItem.show();
    }
    mantraInterval = setInterval(() => {
        mantraIndex = (mantraIndex + 1) % MANTRAS.length;
        if (mantraStatusItem) {
            mantraStatusItem.text = `$(heart) ${MANTRAS[mantraIndex]}`;
        }
    }, 5000);
    vscode.window.showInformationMessage('Mantra Mode activated.');
}

function stopMantra() {
    mantraEnabled = false;
    if (mantraInterval) clearInterval(mantraInterval);
    mantraInterval = null;
    if (mantraStatusItem) mantraStatusItem.hide();
}

// ═══════ Ram Quotes ═══════

function startRamQuotes() {
    if (ramQuoteItem) {
        ramQuoteItem.text = `$(bookmark) ${RAM_QUOTES[quoteIndex]}`;
        ramQuoteItem.show();
    }
    quoteInterval = setInterval(() => {
        quoteIndex = (quoteIndex + 1) % RAM_QUOTES.length;
        if (ramQuoteItem) {
            ramQuoteItem.text = `$(bookmark) ${RAM_QUOTES[quoteIndex]}`;
        }
    }, 12000);
}

function stopRamQuotes() {
    if (quoteInterval) clearInterval(quoteInterval);
    quoteInterval = null;
    if (ramQuoteItem) ramQuoteItem.hide();
}

// ═══════ Activation ═══════

function activate(context) {
    const config = vscode.workspace.getConfiguration('rampyaaryan');

    // ── Neko View Provider (Panel area) ──
    nekoViewProvider = new NekoViewProvider(context.extensionUri);
    context.subscriptions.push(
        vscode.window.registerWebviewViewProvider('rampyaaryan.nekoView', nekoViewProvider, {
            webviewOptions: { retainContextWhenHidden: true }
        })
    );

    // Auto-show Neko panel on first install
    if (config.get('neko.enabled', true)) {
        setTimeout(() => {
            vscode.commands.executeCommand('rampyaaryan.nekoView.focus');
        }, 3000);
    }

    // ── Status Bar Items ──
    hindiStatusItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 200);
    hindiStatusItem.name = 'Rampyaaryan Hindi Line';
    context.subscriptions.push(hindiStatusItem);

    mantraStatusItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 0);
    mantraStatusItem.name = 'Rampyaaryan Mantra';
    mantraStatusItem.tooltip = 'Click for next mantra';
    mantraStatusItem.command = 'rampyaaryan.nextMantra';
    context.subscriptions.push(mantraStatusItem);

    ramQuoteItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, -1);
    ramQuoteItem.name = 'Ram Quote';
    ramQuoteItem.tooltip = 'Sri Ram Quote';
    context.subscriptions.push(ramQuoteItem);

    // ── Commands ──
    context.subscriptions.push(
        vscode.commands.registerCommand('rampyaaryan.openNeko', () => {
            vscode.commands.executeCommand('rampyaaryan.nekoView.focus');
        }),
        vscode.commands.registerCommand('rampyaaryan.toggleNeko', () => {
            const config = vscode.workspace.getConfiguration('rampyaaryan');
            const current = config.get('neko.enabled', true);
            config.update('neko.enabled', !current, vscode.ConfigurationTarget.Global);
            if (!current) {
                vscode.commands.executeCommand('rampyaaryan.nekoView.focus');
                vscode.window.showInformationMessage('Neko Cat enabled! 🐱');
            } else {
                vscode.window.showInformationMessage('Neko Cat disabled.');
            }
        }),
        vscode.commands.registerCommand('rampyaaryan.run', () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) {
                vscode.window.showWarningMessage('Koi file khuli nahi hai! Open a .ram file first.');
                return;
            }
            const filePath = editor.document.fileName;
            if (!filePath.endsWith('.ram')) {
                vscode.window.showWarningMessage('Yeh .ram file nahi hai! This is not a .ram file.');
                return;
            }
            editor.document.save().then(() => {
                let terminal = vscode.window.terminals.find(t => t.name === 'Rampyaaryan');
                if (!terminal) {
                    terminal = vscode.window.createTerminal('Rampyaaryan');
                }
                terminal.show();
                terminal.sendText(`rampyaaryan "${filePath}"`);
            });
        }),
        vscode.commands.registerCommand('rampyaaryan.namaste', () => {
            vscode.window.showInformationMessage(
                'Namaste! Jai Shri Ram! Welcome to Rampyaaryan.'
            );
        }),
        vscode.commands.registerCommand('rampyaaryan.toggleHindiStatusBar', () => {
            hindiBarEnabled = !hindiBarEnabled;
            if (hindiBarEnabled) {
                const editor = vscode.window.activeTextEditor;
                if (editor) updateHindiStatusBar(editor);
                vscode.window.showInformationMessage('Hindi Status Bar enabled.');
            } else {
                if (hindiStatusItem) hindiStatusItem.hide();
                vscode.window.showInformationMessage('Hindi Status Bar disabled.');
            }
        }),
        vscode.commands.registerCommand('rampyaaryan.toggleMantra', () => {
            if (mantraEnabled) {
                stopMantra();
                vscode.window.showInformationMessage('Mantra Mode disabled.');
            } else {
                startMantra();
            }
        }),
        vscode.commands.registerCommand('rampyaaryan.nextMantra', () => {
            if (mantraEnabled) {
                mantraIndex = (mantraIndex + 1) % MANTRAS.length;
                if (mantraStatusItem) {
                    mantraStatusItem.text = `$(heart) ${MANTRAS[mantraIndex]}`;
                }
            }
        }),
        vscode.commands.registerCommand('rampyaaryan.toggleRamQuotes', () => {
            if (quoteInterval) {
                stopRamQuotes();
                vscode.window.showInformationMessage('Ram Quotes disabled.');
            } else {
                startRamQuotes();
                vscode.window.showInformationMessage('Ram Quotes enabled.');
            }
        }),
        vscode.commands.registerCommand('rampyaaryan.setHindiFont', async () => {
            const editorConfig = vscode.workspace.getConfiguration('editor');
            const currentFont = editorConfig.get('fontFamily') || '';
            const hindiFont = 'Noto Sans Devanagari';

            if (currentFont.includes(hindiFont)) {
                vscode.window.showInformationMessage('Hindi font is already set.');
                return;
            }

            const newFont = currentFont ? `${hindiFont}, ${currentFont}` : `${hindiFont}, Consolas, monospace`;
            await editorConfig.update('fontFamily', newFont, vscode.ConfigurationTarget.Global);
            vscode.window.showInformationMessage(`Font set: ${hindiFont} — Devanagari text will render beautifully.`);
        }),
        vscode.commands.registerCommand('rampyaaryan.diwaliMode', () => {
            const messages = [
                '\u0926\u0940\u092A\u093E\u0935\u0932\u0940 \u0915\u0940 \u0939\u093E\u0930\u094D\u0926\u093F\u0915 \u0936\u0941\u092D\u0915\u093E\u092E\u0928\u093E\u090F\u0901!',
                'Happy Diwali from Rampyaaryan!',
                '\u0936\u0941\u092D \u0926\u0940\u092A\u093E\u0935\u0932\u0940 — May your code shine bright!'
            ];
            messages.forEach((msg, i) => {
                setTimeout(() => vscode.window.showInformationMessage(msg), i * 1200);
            });
        }),
        vscode.commands.registerCommand('rampyaaryan.translateToHindi', async () => {
            const currentLang = vscode.env.language;
            if (currentLang === 'hi') {
                const pick = await vscode.window.showInformationMessage(
                    'IDE \u092A\u0939\u0932\u0947 \u0938\u0947 \u0939\u093F\u0928\u094D\u0926\u0940 \u092E\u0947\u0902 \u0939\u0948! English \u092E\u0947\u0902 \u0935\u093E\u092A\u0938 \u091C\u093E\u0928\u093E \u0939\u0948?',
                    'Switch to English', '\u0930\u0939\u0928\u0947 \u0926\u094B'
                );
                if (pick === 'Switch to English') {
                    await vscode.commands.executeCommand('workbench.action.configureLocale', 'en');
                    vscode.window.showInformationMessage('Restart VS Code to switch back to English.');
                }
                return;
            }

            const hindiPack = vscode.extensions.getExtension('MS-CEINTL.vscode-language-pack-hi');
            if (!hindiPack) {
                const install = await vscode.window.showInformationMessage(
                    'Hindi Language Pack install karna hoga. Install karein?',
                    'Install \u0915\u0930\u094B', 'Cancel'
                );
                if (install === 'Install \u0915\u0930\u094B') {
                    await vscode.commands.executeCommand('workbench.extensions.installExtension', 'MS-CEINTL.vscode-language-pack-hi');
                    vscode.window.showInformationMessage(
                        'Hindi Language Pack install ho raha hai... Install hone ke baad VS Code restart hoga.',
                    );
                    setTimeout(async () => {
                        await vscode.commands.executeCommand('workbench.action.configureLocale', 'hi');
                    }, 5000);
                }
                return;
            }

            await vscode.commands.executeCommand('workbench.action.configureLocale', 'hi');
            vscode.window.showInformationMessage(
                '\u092D\u093E\u0937\u093E \u0939\u093F\u0928\u094D\u0926\u0940 \u092E\u0947\u0902 \u092C\u0926\u0932 \u0926\u0940 \u0917\u092F\u0940! VS Code restart \u0915\u0930\u0947\u0902\u0964',
                'Restart Now'
            ).then(choice => {
                if (choice === 'Restart Now') {
                    vscode.commands.executeCommand('workbench.action.reloadWindow');
                }
            });
        })
    );

    // ── Event Listeners ──
    context.subscriptions.push(
        vscode.window.onDidChangeTextEditorSelection(e => {
            if (e.textEditor) {
                updateHindiStatusBar(e.textEditor);
            }
        }),
        vscode.window.onDidChangeActiveTextEditor(editor => {
            if (editor) {
                updateHindiStatusBar(editor);
            }
        })
    );

    // ── Initialize from config ──
    hindiBarEnabled = config.get('hindiStatusBar.enabled', true);
    mantraEnabled = config.get('mantra.enabled', false);

    if (mantraEnabled) {
        startMantra();
    }

    startRamQuotes();

    // ── Initial update ──
    const activeEditor = vscode.window.activeTextEditor;
    if (activeEditor) {
        updateHindiStatusBar(activeEditor);
    }

    // ── Greeting ──
    if (config.get('greeting.enabled', true)) {
        setTimeout(() => {
            vscode.window.showInformationMessage(
                'Namaste! Jai Shri Ram! — Welcome to Rampyaaryan!',
                'Jai Shri Ram',
                'Dismiss'
            ).then(choice => {
                if (choice === 'Jai Shri Ram') {
                    vscode.window.showInformationMessage(
                        'Sri Ram Jay Ram Jay Jay Ram!'
                    );
                }
            });
        }, 2000);
    }
}

function deactivate() {
    if (mantraInterval) clearInterval(mantraInterval);
    if (quoteInterval) clearInterval(quoteInterval);
}

module.exports = { activate, deactivate };
