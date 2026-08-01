const vscode = require('vscode');
const { LanguageClient } = require('vscode-languageclient/node');
const path = require('path');
const fs = require('fs');

let client;

function activate(context) {
    console.log('[Vit Extension] Activating Vit & Vito Language Support...');

    let vitHome = process.env.VIT_HOME || '';
    let lspPath = vitHome ? path.join(vitHome, 'bin', 'vit-lsp.exe') : 'vit-lsp';

    if (!fs.existsSync(lspPath)) {
        lspPath = 'vit-lsp'; // fallback to PATH lookup
    }

    let serverOptions = {
        run: { command: lspPath, args: [] },
        debug: { command: lspPath, args: [] }
    };

    let clientOptions = {
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
        client.start();
        console.log('[Vit Extension] Connected to vit-lsp successfully!');
    } catch (e) {
        console.log('[Vit Extension] LSP launch notice:', e.message);
    }
}

function deactivate() {
    if (!client) {
        return undefined;
    }
    return client.stop();
}

module.exports = {
    activate,
    deactivate
};
