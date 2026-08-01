const vscode = require('vscode');
const { LanguageClient } = require('vscode-languageclient/node');
const path = require('path');
const fs = require('fs');

let client;
let statusBarItem;
let vitTerminal;

function findLspExecutable() {
    const config = vscode.workspace.getConfiguration('vit');
    const customPath = config.get('lsp.path');
    if (customPath && fs.existsSync(customPath)) {
        return customPath;
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
        await client.stop();
        client = null;
    }

    updateStatusBar('starting');
    const lspPath = findLspExecutable();
    console.log(`[Vit Extension] Launching LSP binary from: ${lspPath}`);

    const serverOptions = {
        run: { command: lspPath, args: [] },
        debug: { command: lspPath, args: [] }
    };

    const clientOptions = {
        documentSelector: [{ scheme: 'file', language: 'vit' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.vit')
        }
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
        updateStatusBar('offline');
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
    vitTerminal.sendText(`"${vitBin}" "${filePath}"`);
}

function activate(context) {
    console.log('[Vit Extension] Activating Vit & Vito Language Support v2.2.0...');

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
