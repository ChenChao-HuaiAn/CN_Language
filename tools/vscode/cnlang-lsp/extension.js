const path = require('path');
const vscode = require('vscode');
const {
    LanguageClient,
    LanguageClientOptions,
    CloseAction,
    ErrorAction
} = require('vscode-languageclient/node');

/** @type {LanguageClient | undefined} */
let client;

/**
 * 激活扩展
 * @param {vscode.ExtensionContext} context
 */
function activate(context) {
    const config = vscode.workspace.getConfiguration('cnlang');
    let serverPath = config.get('lsp.serverPath');

    if (!serverPath) {
        const workspaceFolders = vscode.workspace.workspaceFolders;
        if (!workspaceFolders || workspaceFolders.length === 0) {
            vscode.window.showWarningMessage('CN_Language LSP: 当前没有打开的工作区，无法自动定位 cnlsp 路径，请在设置中配置 cnlang.lsp.serverPath。');
        } else {
            const root = workspaceFolders[0].uri.fsPath;
            if (process.platform === 'win32') {
                serverPath = path.join(root, 'build', 'src', 'Debug', 'cnlsp.exe');
            } else {
                serverPath = path.join(root, 'build', 'src', 'cnlsp');
            }
        }
    }

    if (!serverPath) {
        vscode.window.showErrorMessage('CN_Language LSP: 未能确定 cnlsp 可执行文件路径，请在设置中配置 cnlang.lsp.serverPath。');
        return;
    }

    const serverOptions = {
        command: serverPath,
        args: [],
    };

    /** @type {LanguageClientOptions} */
    const clientOptions = {
        documentSelector: [
            { language: 'cn', scheme: 'file' }
        ],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.cn')
        },
        errorHandler: {
            error: () => ({ action: ErrorAction.Continue }),
            closed: () => ({ action: CloseAction.Restart })
        }
    };

    client = new LanguageClient(
        'cnlangLsp',
        'CN_Language Language Server',
        serverOptions,
        clientOptions
    );

    context.subscriptions.push(client.start());
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
