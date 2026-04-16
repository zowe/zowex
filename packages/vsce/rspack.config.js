/**
 * This program and the accompanying materials are made available under the terms of the
 * Eclipse Public License v2.0 which accompanies this distribution, and is available at
 * https://www.eclipse.org/legal/epl-v20.html
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Copyright Contributors to the Zowe Project.
 *
 */

//@ts-check

'use strict';

const path = require('path');
const rspack = require('@rspack/core');
const { TsCheckerRspackPlugin } = require('ts-checker-rspack-plugin');

module.exports = (_env, argv) => {
  const isProd = argv.mode === 'production';

  /**@type {import('@rspack/core').RspackOptions}*/
  const extensionConfig = {
    target: 'node', // VS Code extensions run in a Node.js-context 📖 -> https://webpack.js.org/configuration/node/
    entry: './src/extension.ts', // the entry point of this extension, 📖 -> https://webpack.js.org/configuration/entry-context/
    output: {
      // the bundle is stored in the 'dist' folder (check package.json), 📖 -> https://webpack.js.org/configuration/output/
      path: path.resolve(__dirname, 'out'),
      filename: 'extension.js',
      libraryTarget: 'commonjs2',
      devtoolModuleFilenameTemplate: 'webpack:///[absolute-resource-path]'
    },
    devtool: isProd ? 'nosources-source-map' : 'source-map',
    externals: {
      vscode: 'commonjs vscode' // the vscode-module is created on-the-fly and must be excluded. Add other modules that cannot be webpack'ed, 📖 -> https://webpack.js.org/configuration/externals/
      // modules added here also need to be added in the .vscodeignore file
    },
    resolve: {
      // support reading TypeScript and JavaScript files, 📖 -> https://github.com/TypeStrong/ts-loader
      extensions: ['.ts', '.js'],
      alias: {
        'zowex-sdk': path.resolve(__dirname, '..', 'sdk', 'src'),
        'cpu-features': false,
        './crypto/build/Release/sshcrypto.node': false,
      }
    },
    module: {
      rules: [
        {
          test: /\.ts$/,
          exclude: /node_modules/,
          loader: 'builtin:swc-loader',
          options: {
            jsc: {
              parser: {
                syntax: 'typescript',
              },
              target: 'es2022'
            },
          },
        },
        {
          // Patch russh/lib/native.js to load .node binaries from prebuilds/
          // using __non_webpack_require__ so rspack doesn't try to bundle them.
          test: /russh[\\/]lib[\\/]native\.js$/,
          loader: path.resolve(__dirname, 'russh-native-loader.js'),
        },
      ]
    },
    plugins: [
      new TsCheckerRspackPlugin(),
      new rspack.CopyRspackPlugin({
        patterns: [{ from: '../sdk/bin', to: '../bin', noErrorOnMissing: !isProd, force: true }],
      }),
    ],
    stats: {
      warnings: false,
    }
  };

  return [extensionConfig];
};
