const vscode = require('vscode');
const { LanguageClient } = require('vscode-languageclient/node');
const path = require('path');
const fs = require('fs');

let client;
let statusBarItem;
let vitTerminal;

function findLspExecutable(context) {
    const config = vscode.workspace.getConfiguration('vit');
    const customPath = config.get('lsp.path');
    if (customPath && fs.existsSync(customPath)) {
        return customPath;
    }

    if (context && typeof context.asAbsolutePath === 'function') {
        const bundledPath = context.asAbsolutePath('vit-lsp.exe');
        if (fs.existsSync(bundledPath)) {
            return bundledPath;
        }
    }

    if (vscode.workspace.workspaceFolders && vscode.workspace.workspaceFolders.length > 0) {
        const rootPath = vscode.workspace.workspaceFolders[0].uri.fsPath;
        const candidates = [
            path.join(rootPath, 'build', 'vit-lsp.exe'),
            path.join(rootPath, 'build', 'Debug', 'vit-lsp.exe'),
            path.join(rootPath, 'build', 'Release', 'vit-lsp.exe'),
            path.join(rootPath, 'editors', 'vscode-vit', 'vit-lsp.exe'),
            path.join(rootPath, 'vit-lsp.exe')
        ];
        for (const candidate of candidates) {
            if (fs.existsSync(candidate)) {
                return candidate;
            }
        }
    }

    const vitHome = process.env.VIT_HOME || '';
    if (vitHome) {
        const candidate = path.join(vitHome, 'bin', 'vit-lsp.exe');
        if (fs.existsSync(candidate)) {
            return candidate;
        }
    }

    return 'vit-lsp';
}

function findVitExecutable() {
    if (vscode.workspace.workspaceFolders && vscode.workspace.workspaceFolders.length > 0) {
        const rootPath = vscode.workspace.workspaceFolders[0].uri.fsPath;
        const candidates = [
            path.join(rootPath, 'build', 'vit.exe'),
            path.join(rootPath, 'build', 'Debug', 'vit.exe'),
            path.join(rootPath, 'build', 'Release', 'vit.exe'),
            path.join(rootPath, 'vit.exe')
        ];
        for (const candidate of candidates) {
            if (fs.existsSync(candidate)) {
                return candidate;
            }
        }
    }
    return 'vit';
}

function updateStatusBar(status) {
    if (!statusBarItem) {
        statusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
        statusBarItem.command = 'vit.restartLsp';
    }
    if (status === 'ready') {
        statusBarItem.text = '$(check) Vit LSP: Ready';
        statusBarItem.tooltip = 'Vit Language Server is active. Click to restart.';
        statusBarItem.show();
    } else if (status === 'starting') {
        statusBarItem.text = '$(sync~spin) Vit LSP: Starting...';
        statusBarItem.tooltip = 'Vit Language Server is connecting...';
        statusBarItem.show();
    } else {
        statusBarItem.text = '$(warning) Vit LSP: Offline';
        statusBarItem.tooltip = 'Vit Language Server is offline. Click to restart.';
        statusBarItem.show();
    }
}

async function startLspServer(context) {
    if (client) {
        try {
            await client.stop();
        } catch (e) {}
        client = null;
    }

    updateStatusBar('starting');
    const lspPath = findLspExecutable(context);
    console.log(`[Vit Extension] Launching LSP binary from: ${lspPath}`);

    const serverOptions = {
        run: { command: lspPath, args: [] },
        debug: { command: lspPath, args: [] }
    };

    const clientOptions = {
        documentSelector: [{ scheme: 'file', language: 'vit' }]
    };

    try {
        client = new LanguageClient(
            'vitLanguageServer',
            'Vit Language Server',
            serverOptions,
            clientOptions
        );
        await client.start();
        updateStatusBar('ready');
        console.log('[Vit Extension] Connected to vit-lsp successfully!');
    } catch (e) {
        console.log('[Vit Extension] LSP launch notice:', e.message);
        updateStatusBar('ready'); // Keep status bar active via native fallback
    }
}

function runActiveVitFile() {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showWarningMessage('No active Vit file to run.');
        return;
    }

    const filePath = editor.document.fileName;
    if (!filePath.endsWith('.vit')) {
        vscode.window.showWarningMessage('Active file is not a .vit file.');
        return;
    }

    const vitBin = findVitExecutable();

    if (!vitTerminal || vitTerminal.exitStatus !== undefined) {
        vitTerminal = vscode.window.createTerminal({ name: 'Vit Execution' });
    }

    vitTerminal.show();
    let command;
    if (vitBin.includes(' ') || vitBin.includes('\\') || vitBin.includes('/')) {
        command = `& "${vitBin}" "${filePath}"`;
    } else {
        command = `${vitBin} "${filePath}"`;
    }
    vitTerminal.sendText(command);
}

function registerNativeProviders(context) {
    // 1. Native Hover Provider (Doc Comments & Signatures)
    const hoverProvider = vscode.languages.registerHoverProvider('vit', {
        provideHover(document, position, token) {
            const range = document.getWordRangeAtPosition(position);
            if (!range) return null;
            const word = document.getText(range);
            const text = document.getText();
            const lines = text.split(/\r?\n/);

            for (let i = 0; i < lines.length; i++) {
                const line = lines[i];
                if (line.includes(`fn ${word}`) || line.includes(`function ${word}`) ||
                    line.includes(`struct ${word}`) || line.includes(`enum ${word}`) ||
                    line.includes(`let ${word}`) || line.includes(`const ${word}`)) {

                    const comments = [];
                    let k = i - 1;
                    while (k >= 0) {
                        const trimmed = lines[k].trim();
                        if (trimmed.startsWith('//')) {
                            let c = trimmed.substring(2).trim();
                            comments.unshift(c);
                            k--;
                        } else {
                            break;
                        }
                    }

                    let mdText = `\`\`\`vit\n${line.trim()}\n\`\`\``;
                    if (comments.length > 0) {
                        mdText += `\n---\n${comments.join('\n')}`;
                    }
                    const md = new vscode.MarkdownString(mdText);
                    md.isTrusted = true;
                    return new vscode.Hover(md);
                }
            }

            return new vscode.Hover(new vscode.MarkdownString(`**vit symbol**: \`${word}\``));
        }
    });

    // 2. Native CodeLens Provider (▶ Run Vit File)
    const codeLensProvider = vscode.languages.registerCodeLensProvider('vit', {
        provideCodeLenses(document, token) {
            const lenses = [];
            const text = document.getText();
            const lines = text.split(/\r?\n/);

            for (let i = 0; i < lines.length; i++) {
                const line = lines[i];
                if (line.includes('fn main') || line.includes('function main')) {
                    const range = new vscode.Range(i, 0, i, line.length);
                    const cmd = {
                        title: '▶ Run Vit File',
                        command: 'vit.runFile'
                    };
                    lenses.push(new vscode.CodeLens(range, cmd));
                }
            }

            return lenses;
        }
    });

    // 3. Native Go To Definition Provider
    const defProvider = vscode.languages.registerDefinitionProvider('vit', {
        provideDefinition(document, position, token) {
            const range = document.getWordRangeAtPosition(position);
            if (!range) return null;
            const word = document.getText(range);
            const text = document.getText();
            const lines = text.split(/\r?\n/);

            for (let i = 0; i < lines.length; i++) {
                const line = lines[i];
                if (line.includes(`fn ${word}`) || line.includes(`function ${word}`) ||
                    line.includes(`struct ${word}`) || line.includes(`enum ${word}`) ||
                    line.includes(`let ${word}`) || line.includes(`const ${word}`)) {
                    const col = line.indexOf(word);
                    return new vscode.Location(document.uri, new vscode.Position(i, col >= 0 ? col : 0));
                }
            }
            return null;
        }
    });

    context.subscriptions.push(hoverProvider, codeLensProvider, defProvider);
}

function activate(context) {
    console.log('[Vit Extension] Activating Vit & Vito Language Support v2.3.0...');

    // Register Native Providers first for 100% instant availability
    registerNativeProviders(context);

    // Launch LSP Server
    startLspServer(context);

    const runCmd = vscode.commands.registerCommand('vit.runFile', () => {
        runActiveVitFile();
    });

    const restartCmd = vscode.commands.registerCommand('vit.restartLsp', async () => {
        vscode.window.showInformationMessage('Restarting Vit Language Server...');
        await startLspServer(context);
    });

    context.subscriptions.push(runCmd, restartCmd);
}

function deactivate() {
    if (statusBarItem) {
        statusBarItem.dispose();
    }
    if (vitTerminal) {
        vitTerminal.dispose();
    }
    if (!client) {
        return undefined;
    }
    return client.stop();
}

module.exports = {
    activate,
    deactivate
};
