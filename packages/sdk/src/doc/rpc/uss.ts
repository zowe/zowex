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

import type { Readable, Writable } from "node:stream";
import type { B64String } from "../types";
import type * as common from "./common";

export interface ChmodFileRequest extends common.CommandRequest<"chmodFile"> {
    /**
     * Desired permissions for the file (represented as an octal value, e.g. "755")
     */
    mode: string;
    /**
     * Remote file path to change permissions for
     */
    fspath: string;
    /**
     * Whether to change permissions recursively
     */
    recursive?: boolean;
}

export type ChmodFileResponse = common.CommandResponse;

export interface ChownFileRequest extends common.CommandRequest<"chownFile"> {
    /**
     * New owner for the file
     */
    owner: string;
    /**
     * Remote file path to change ownership for
     */
    fspath: string;
    /**
     * Whether to apply ownership to inner files and directories
     */
    recursive?: boolean;
}

export type ChownFileResponse = common.CommandResponse;

export interface ChtagFileRequest extends common.CommandRequest<"chtagFile"> {
    /**
     * Remote file path to change tags for
     */
    fspath: string;
    /**
     * New tag for the file
     */
    tag: string;
    /**
     * Whether to apply the tag to inner files and directories
     */
    recursive?: boolean;
}

export type ChtagFileResponse = common.CommandResponse;

export interface CopyUssRequest extends common.CommandRequest<"copyUss"> {
    /**
     * The source file or directory to copy.
     */
    srcFsPath: string;
    /**
     * The destination file or directory to copy into.
     */
    dstFsPath: string;
    /**
     * Should the copy action recursively traverse through sub-directories and files.
     *
     * Default: false
     */
    recursive?: boolean;
    /**
     * Should the copy action follow symlinks when encountered. Note: This can only be set in combination with recursive.
     *
     * Default: false
     */
    followSymlinks?: boolean;

    /**
     * Set to true if the copy action should preserve permission bits and ownership in the output destination.
     *   Note: the USS 'cp' utility by default does not preserve these permissions.
     *
     * Default: false
     */
    preserveAttributes?: boolean;

    /**
     * Set to true if the copy action should try and replace any files it cannot open in the output destination.
     *  Equivalent to 'cp -f'
     *
     * Default: false
     */
    force?: boolean;
}

export type CopyUssResponse = common.CommandResponse;

export interface CreateFileRequest extends common.CommandRequest<"createFile"> {
    /**
     * Permissions for the new path
     */
    permissions?: string;
    /**
     * Remote file path to create
     */
    fspath: string;
    /**
     * Whether to create a directory (true) or a file (false)
     */
    isDir?: boolean;
    /**
     * Whether to overwrite existing file
     */
    overwrite?: boolean;
}

export type CreateFileResponse = common.CommandResponse;

export interface DeleteFileRequest extends common.CommandRequest<"deleteFile"> {
    /**
     * Remote file path to delete
     */
    fspath: string;
    /**
     * Whether to delete the file recursively
     */
    recursive?: boolean;
}

export type DeleteFileResponse = common.CommandResponse;

export interface ListFilesRequest extends common.CommandRequest<"listFiles">, common.ListOptions {
    /**
     * Directory to list files for
     */
    fspath: string;
    /**
     * Whether to include hidden files
     */
    all?: boolean;
    /**
     * Whether to return the long format with all attributes
     */
    long?: boolean;
    /**
     * Depth of subdirectories to list (defaults to 1)
     */
    depth?: number;
}

export interface ListFilesResponse extends common.CommandResponse {
    /**
     * List of returned files
     */
    items: common.UssItem[];
    /**
     * Number of rows returned
     */
    returnedRows: number;
}

export interface ReadFileRequest extends common.CommandRequest<"readFile">, common.WritableStreamRpc {
    /**
     * Desired encoding for the file (optional)
     */
    encoding?: string;
    /**
     * Source encoding of the file content (optional, defaults to UTF-8)
     */
    localEncoding?: string;
    /**
     * Remote file path to read contents from
     */
    fspath: string;
    /**
     * Stream to write contents to
     */
    stream?: () => Writable;
}

export interface ReadFileResponse extends common.CommandResponse {
    /**
     * Returned encoding for the file
     */
    encoding?: string;
    /**
     * Returned e-tag for the file
     */
    etag: string;
    /**
     * File contents (omitted if streaming)
     */
    data: B64String;
    /**
     * Length of file contents in bytes (only used for streaming)
     */
    contentLen?: number;
}

export interface WriteFileRequest extends common.CommandRequest<"writeFile">, common.ReadableStreamRpc {
    /**
     * Desired encoding for the file (optional)
     */
    encoding?: string;
    /**
     * Source encoding of the file content (optional, defaults to UTF-8)
     */
    localEncoding?: string;
    /**
     * E-tag for the file to detect conflicts during save (optional)
     */
    etag?: string;
    /**
     * Remote file path to write contents to
     */
    fspath: string;
    /**
     * New contents for the file
     */
    data?: B64String;
    /**
     * Stream to read contents from
     */
    stream?: () => Readable;
    /**
     * Length of file contents in bytes (only used for streaming)
     */
    contentLen?: number;
}

export interface WriteFileResponse extends common.CommandResponse {
    /**
     * Returned e-tag for the file
     */
    etag: string;
    /**
     * Whether new file was created
     */
    created: boolean;
    /**
     * Length of file contents in bytes (only used for streaming)
     */
    contentLen?: number;
}
export interface IssueUssCmdRequest extends common.CommandRequest<"unixCommand"> {
    /**
     * UNIX command to execute
     */
    commandText: string;
}

export interface IssueUssCmdResponse extends common.CommandResponse {
    /**
     * Data returned from the UNIX command
     */
    data: string;
}

export interface MoveFileRequest extends common.CommandRequest<"moveFile"> {
    /**
     * Source path to move
     */
    source: string;
    /**
     * Target path to move to
     */
    target: string;
    /**
     * Whether to force the move
     */
    force?: boolean; // default: true
}

export type MoveFileResponse = common.CommandResponse;
