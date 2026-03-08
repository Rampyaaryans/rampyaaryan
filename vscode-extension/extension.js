const vscode = require('vscode');

// ════════════════════════════════════════════════════════════════
//   🐱 Neko Cat  ·  🙏 Hindi Status Bar  ·  📿 Mantra Mode
// ════════════════════════════════════════════════════════════════

const HINDI_DIGITS = ['०','१','२','३','४','५','६','७','८','९'];

const CAT_FACES = {
    idle:     '😺',
    moving:   '🐱',
    typing:   '😸',
    sleeping: '😴',
    happy:    '😻',
    shocked:  '🙀',
    love:     '😽'
};

const MANTRAS = [
    'ॐ नमः शिवाय 🙏',
    'जय श्री राम 🙏',
    'हरे कृष्ण हरे राम',
    'सत्यमेव जयते',
    'वसुधैव कुटुम्बकम्',
    'कर्मण्येवाधिकारस्ते',
    'ॐ जय जगदीश हरे',
    'श्री राम जय राम जय जय राम',
    'ॐ शांति शांति शांति',
    'सर्वे भवन्तु सुखिनः',
    'तमसो मा ज्योतिर्गमय',
    'योगः कर्मसु कौशलम्'
];

const RAM_QUOTES = [
    '🏹 धर्म की जय हो',
    '🪷 राम नाम सत्य है',
    '🙏 बोलो सियापति रामचन्द्र की जय',
    '🕉️ रघुपति राघव राजा राम',
    '📿 मर्यादा पुरुषोत्तम',
    '🏹 सत्य का मार्ग ही धर्म है',
    '🪷 जय सियाराम',
    '🙏 मंगल भवन अमंगल हारी'
];

// ═══════ State ═══════
let nekoCatEnabled = false;
let hindiBarEnabled = true;
let mantraEnabled = false;

let hindiStatusItem = null;
let mantraStatusItem = null;
let catStatusItem = null;
let ramQuoteItem = null;

let catDecorationTypes = {};
let currentCatState = 'idle';
let idleTimeout = null;
let sleepTimeout = null;
let mantraInterval = null;
let mantraIndex = 0;
let quoteInterval = null;
let quoteIndex = 0;
let lastCursorLine = -1;
let lastCursorCol = -1;

// ═══════ Helpers ═══════

function toHindiNumerals(n) {
    return String(n).split('').map(d => HINDI_DIGITS[parseInt(d)] || d).join('');
}

// ═══════ Neko Cat ═══════

function initCatDecorations() {
    disposeCatDecorations();
    for (const [state, emoji] of Object.entries(CAT_FACES)) {
        catDecorationTypes[state] = vscode.window.createTextEditorDecorationType({
            after: {
                contentText: ` ${emoji}`,
                margin: '0 0 0 4px',
                textDecoration: 'none; position: relative; top: -1px;'
            }
        });
    }
}

function disposeCatDecorations() {
    for (const dt of Object.values(catDecorationTypes)) {
        dt.dispose();
    }
    catDecorationTypes = {};
}

function updateCatInEditor(editor) {
    if (!nekoCatEnabled || !editor) return;

    // Clear all states from editor
    for (const dt of Object.values(catDecorationTypes)) {
        editor.setDecorations(dt, []);
    }

    // Place current cat at cursor
    const pos = editor.selection.active;
    const dt = catDecorationTypes[currentCatState];
    if (dt) {
        editor.setDecorations(dt, [new vscode.Range(pos, pos)]);
    }

    // Update status bar
    if (catStatusItem) {
        catStatusItem.text = `${CAT_FACES[currentCatState]} नेको`;
        catStatusItem.show();
    }
}

function setCatState(state, editor) {
    currentCatState = state;
    updateCatInEditor(editor);
}

function onCursorMove(editor) {
    if (!nekoCatEnabled || !editor) return;

    const pos = editor.selection.active;
    const moved = pos.line !== lastCursorLine || pos.character !== lastCursorCol;
    lastCursorLine = pos.line;
    lastCursorCol = pos.character;

    if (moved) {
        setCatState('moving', editor);

        // Reset idle timer
        if (idleTimeout) clearTimeout(idleTimeout);
        if (sleepTimeout) clearTimeout(sleepTimeout);

        idleTimeout = setTimeout(() => {
            setCatState('idle', editor);
            sleepTimeout = setTimeout(() => {
                setCatState('sleeping', editor);
            }, 8000);
        }, 2000);
    }
}

function onTextChange(editor) {
    if (!nekoCatEnabled || !editor) return;

    setCatState('typing', editor);

    if (idleTimeout) clearTimeout(idleTimeout);
    if (sleepTimeout) clearTimeout(sleepTimeout);

    idleTimeout = setTimeout(() => {
        // Random happy/love reaction after typing stops
        const reaction = Math.random() > 0.7 ? 'love' : Math.random() > 0.5 ? 'happy' : 'idle';
        setCatState(reaction, editor);
        sleepTimeout = setTimeout(() => {
            setCatState('sleeping', editor);
        }, 8000);
    }, 1500);
}

function enableNekoCat() {
    nekoCatEnabled = true;
    initCatDecorations();
    const editor = vscode.window.activeTextEditor;
    if (editor) {
        setCatState('happy', editor);
        setTimeout(() => setCatState('idle', editor), 2000);
    }
    if (catStatusItem) catStatusItem.show();
    vscode.window.showInformationMessage('🐱 नेको बिल्ली चालू! Neko Cat is following your cursor!');
}

function disableNekoCat() {
    nekoCatEnabled = false;
    if (idleTimeout) clearTimeout(idleTimeout);
    if (sleepTimeout) clearTimeout(sleepTimeout);
    const editor = vscode.window.activeTextEditor;
    if (editor) {
        for (const dt of Object.values(catDecorationTypes)) {
            editor.setDecorations(dt, []);
        }
    }
    disposeCatDecorations();
    if (catStatusItem) catStatusItem.hide();
    vscode.window.showInformationMessage('😿 नेको बिल्ली बंद। Neko Cat disabled.');
}

// ═══════ Hindi Status Bar ═══════

function updateHindiStatusBar(editor) {
    if (!hindiBarEnabled || !hindiStatusItem || !editor) {
        if (hindiStatusItem) hindiStatusItem.hide();
        return;
    }
    const pos = editor.selection.active;
    const line = toHindiNumerals(pos.line + 1);
    const col = toHindiNumerals(pos.character + 1);
    hindiStatusItem.text = `$(book) पंक्ति ${line} · स्तंभ ${col}`;
    hindiStatusItem.tooltip = `Line ${pos.line + 1}, Column ${pos.character + 1} — हिन्दी में`;
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
    vscode.window.showInformationMessage('📿 मंत्र मोड चालू! Mantra Mode activated.');
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
        ramQuoteItem.text = RAM_QUOTES[quoteIndex];
        ramQuoteItem.show();
    }
    quoteInterval = setInterval(() => {
        quoteIndex = (quoteIndex + 1) % RAM_QUOTES.length;
        if (ramQuoteItem) {
            ramQuoteItem.text = RAM_QUOTES[quoteIndex];
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

    // ── Status Bar Items ──
    hindiStatusItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 200);
    hindiStatusItem.name = 'Rampyaaryan Hindi Line';
    context.subscriptions.push(hindiStatusItem);

    mantraStatusItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 0);
    mantraStatusItem.name = 'Rampyaaryan Mantra';
    mantraStatusItem.tooltip = 'क्लिक करें अगला मंत्र देखने के लिए — Click for next mantra';
    mantraStatusItem.command = 'rampyaaryan.nextMantra';
    context.subscriptions.push(mantraStatusItem);

    catStatusItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 150);
    catStatusItem.name = 'Neko Cat';
    catStatusItem.tooltip = 'नेको बिल्ली — Neko Cat';
    context.subscriptions.push(catStatusItem);

    ramQuoteItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, -1);
    ramQuoteItem.name = 'Ram Quote';
    ramQuoteItem.tooltip = 'श्री राम वचन — Sri Ram Quote';
    context.subscriptions.push(ramQuoteItem);

    // ── Commands ──
    context.subscriptions.push(
        vscode.commands.registerCommand('rampyaaryan.enableNekoCat', () => enableNekoCat()),
        vscode.commands.registerCommand('rampyaaryan.disableNekoCat', () => disableNekoCat()),
        vscode.commands.registerCommand('rampyaaryan.namaste', () => {
            vscode.window.showInformationMessage(
                '🙏 नमस्ते! जय श्री राम!\n' +
                'रामप्यारयन में आपका स्वागत है!\n' +
                'Welcome to Rampyaaryan — India\'s First Hinglish Programming Language!'
            );
        }),
        vscode.commands.registerCommand('rampyaaryan.toggleHindiStatusBar', () => {
            hindiBarEnabled = !hindiBarEnabled;
            if (hindiBarEnabled) {
                const editor = vscode.window.activeTextEditor;
                if (editor) updateHindiStatusBar(editor);
                vscode.window.showInformationMessage('📊 हिन्दी स्टेटस बार चालू!');
            } else {
                if (hindiStatusItem) hindiStatusItem.hide();
                vscode.window.showInformationMessage('📊 Hindi Status Bar disabled.');
            }
        }),
        vscode.commands.registerCommand('rampyaaryan.toggleMantra', () => {
            if (mantraEnabled) {
                stopMantra();
                vscode.window.showInformationMessage('📿 मंत्र मोड बंद। Mantra Mode disabled.');
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
                vscode.window.showInformationMessage('Ram quotes disabled.');
            } else {
                startRamQuotes();
                vscode.window.showInformationMessage('🏹 श्री राम वचन चालू! Ram Quotes enabled.');
            }
        }),
        vscode.commands.registerCommand('rampyaaryan.setHindiFont', async () => {
            const editorConfig = vscode.workspace.getConfiguration('editor');
            const currentFont = editorConfig.get('fontFamily') || '';
            const hindiFont = 'Noto Sans Devanagari';

            if (currentFont.includes(hindiFont)) {
                vscode.window.showInformationMessage('हिन्दी फॉन्ट पहले से सेट है! Hindi font is already set.');
                return;
            }

            const newFont = currentFont ? `${hindiFont}, ${currentFont}` : `${hindiFont}, Consolas, monospace`;
            await editorConfig.update('fontFamily', newFont, vscode.ConfigurationTarget.Global);
            vscode.window.showInformationMessage(`🔤 फॉन्ट सेट: ${hindiFont} — Hindi/Devanagari text will now render beautifully!`);
        }),
        vscode.commands.registerCommand('rampyaaryan.diwaliMode', () => {
            const messages = [
                '🪔 दीपावली की हार्दिक शुभकामनाएँ!',
                '✨ Happy Diwali from Rampyaaryan!',
                '🎆 शुभ दीपावली — May your code shine bright!',
                '🪔🪔🪔🪔🪔🪔🪔🪔🪔🪔🪔🪔🪔🪔🪔'
            ];
            messages.forEach((msg, i) => {
                setTimeout(() => vscode.window.showInformationMessage(msg), i * 1200);
            });
        })
    );

    // ── Event Listeners ──
    context.subscriptions.push(
        vscode.window.onDidChangeTextEditorSelection(e => {
            if (e.textEditor) {
                updateHindiStatusBar(e.textEditor);
                onCursorMove(e.textEditor);
            }
        }),
        vscode.workspace.onDidChangeTextDocument(e => {
            const editor = vscode.window.activeTextEditor;
            if (editor && e.document === editor.document) {
                onTextChange(editor);
            }
        }),
        vscode.window.onDidChangeActiveTextEditor(editor => {
            if (editor) {
                updateHindiStatusBar(editor);
                if (nekoCatEnabled) {
                    updateCatInEditor(editor);
                }
            }
        })
    );

    // ── Initialize from config ──
    nekoCatEnabled = config.get('nekoCat.enabled', false);
    hindiBarEnabled = config.get('hindiStatusBar.enabled', true);
    mantraEnabled = config.get('mantra.enabled', false);

    if (nekoCatEnabled) {
        initCatDecorations();
    }
    if (mantraEnabled) {
        startMantra();
    }

    // Start Ram quotes
    startRamQuotes();

    // ── Initial update ──
    const activeEditor = vscode.window.activeTextEditor;
    if (activeEditor) {
        updateHindiStatusBar(activeEditor);
        if (nekoCatEnabled) {
            setCatState('happy', activeEditor);
        }
    }

    // ── Greeting ──
    if (config.get('greeting.enabled', true)) {
        setTimeout(() => {
            vscode.window.showInformationMessage(
                '🙏 नमस्ते! जय श्री राम! — Welcome to Rampyaaryan!',
                'जय श्री राम 🚩',
                'Dismiss'
            ).then(choice => {
                if (choice === 'जय श्री राम 🚩') {
                    vscode.window.showInformationMessage(
                        '🏹 बोलो सियापति रामचन्द्र की जय! 🚩\n' +
                        'श्री राम जय राम जय जय राम!'
                    );
                }
            });
        }, 2000);
    }
}

function deactivate() {
    if (idleTimeout) clearTimeout(idleTimeout);
    if (sleepTimeout) clearTimeout(sleepTimeout);
    if (mantraInterval) clearInterval(mantraInterval);
    if (quoteInterval) clearInterval(quoteInterval);
    disposeCatDecorations();
}

module.exports = { activate, deactivate };
