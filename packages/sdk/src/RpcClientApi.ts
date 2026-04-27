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

import type { IRpcClient } from "./doc/client";
import type { CommandRequest, CommandResponse, console, core, ds, jobs, tool, tso, uss } from "./doc/rpc";
import type { ProgressCallback } from "./doc/types";

export abstract class RpcClientApi implements IRpcClient {
    public abstract request<ReqT extends CommandRequest, RespT extends CommandResponse>(
        request: ReqT,
        progressCallback?: (percent: number) => void,
    ): Promise<RespT>;

    public console = {
        issueCmd: this.rpc<console.IssueConsoleCmdRequest, console.IssueConsoleCmdResponse>("consoleCommand"),
    };

    public core = {
        getInfo: this.rpc<core.GetInfoRequest, core.GetInfoResponse>("getInfo"),
    };

    public ds = {
        createDataset: this.rpc<ds.CreateDatasetRequest, ds.CreateDatasetResponse>("createDataset"),
        createMember: this.rpc<ds.CreateMemberRequest, ds.CreateMemberResponse>("createMember"),
        deleteDataset: this.rpc<ds.DeleteDatasetRequest, ds.DeleteDatasetResponse>("deleteDataset"),
        listDatasets: this.rpc<ds.ListDatasetsRequest, ds.ListDatasetsResponse>("listDatasets"),
        listDsMembers: this.rpc<ds.ListDsMembersRequest, ds.ListDsMembersResponse>("listDsMembers"),
        readDataset: this.rpc<ds.ReadDatasetRequest, ds.ReadDatasetResponse>("readDataset"),
        restoreDataset: this.rpc<ds.RestoreDatasetRequest, ds.RestoreDatasetResponse>("restoreDataset"),
        writeDataset: this.rpc<ds.WriteDatasetRequest, ds.WriteDatasetResponse>("writeDataset"),
        renameDataset: this.rpc<ds.RenameDatasetRequest, ds.RenameDatasetResponse>("renameDataset"),
        renameMember: this.rpc<ds.RenameMemberRequest, ds.RenameMemberResponse>("renameMember"),
        copyDatasetOrMember: this.rpc<ds.CopyDatasetRequest, ds.CopyDatasetResponse>("copyDatasetOrMember"),
    };

    public jobs = {
        cancelJob: this.rpc<jobs.CancelJobRequest, jobs.CancelJobResponse>("cancelJob"),
        deleteJob: this.rpc<jobs.DeleteJobRequest, jobs.DeleteJobResponse>("deleteJob"),
        getJcl: this.rpc<jobs.GetJclRequest, jobs.GetJclResponse>("getJcl"),
        getStatus: this.rpc<jobs.GetJobStatusRequest, jobs.GetJobStatusResponse>("getJobStatus"),
        holdJob: this.rpc<jobs.HoldJobRequest, jobs.HoldJobResponse>("holdJob"),
        listJobs: this.rpc<jobs.ListJobsRequest, jobs.ListJobsResponse>("listJobs"),
        listSpools: this.rpc<jobs.ListSpoolsRequest, jobs.ListSpoolsResponse>("listSpools"),
        readSpool: this.rpc<jobs.ReadSpoolRequest, jobs.ReadSpoolResponse>("readSpool"),
        releaseJob: this.rpc<jobs.ReleaseJobRequest, jobs.ReleaseJobResponse>("releaseJob"),
        submitJcl: this.rpc<jobs.SubmitJclRequest, jobs.SubmitJclResponse>("submitJcl"),
        submitJob: this.rpc<jobs.SubmitJobRequest, jobs.SubmitJobResponse>("submitJob"),
        submitUss: this.rpc<jobs.SubmitUssRequest, jobs.SubmitUssResponse>("submitUss"),
    };

    public tool = {
        search: this.rpc<tool.ToolSearchRequest, tool.ToolSearchResponse>("toolSearch"),
    };

    public tso = {
        issueCmd: this.rpc<tso.IssueTsoCmdRequest, tso.IssueTsoCmdResponse>("tsoCommand"),
    };

    public uss = {
        copyUss: this.rpc<uss.CopyUssRequest, uss.CopyUssResponse>("copyUss"),
        chmodFile: this.rpc<uss.ChmodFileRequest, uss.ChmodFileResponse>("chmodFile"),
        chownFile: this.rpc<uss.ChownFileRequest, uss.ChownFileResponse>("chownFile"),
        chtagFile: this.rpc<uss.ChtagFileRequest, uss.ChtagFileResponse>("chtagFile"),
        createFile: this.rpc<uss.CreateFileRequest, uss.CreateFileResponse>("createFile"),
        deleteFile: this.rpc<uss.DeleteFileRequest, uss.DeleteFileResponse>("deleteFile"),
        listFiles: this.rpc<uss.ListFilesRequest, uss.ListFilesResponse>("listFiles"),
        readFile: this.rpcWithProgress<uss.ReadFileRequest, uss.ReadFileResponse>("readFile"),
        writeFile: this.rpcWithProgress<uss.WriteFileRequest, uss.WriteFileResponse>("writeFile"),
        issueCmd: this.rpc<uss.IssueUssCmdRequest, uss.IssueUssCmdResponse>("unixCommand"),
        moveFile: this.rpc<uss.MoveFileRequest, uss.MoveFileResponse>("moveFile"),
    };

    private rpc<ReqT extends CommandRequest, RespT extends CommandResponse>(command: ReqT["command"]) {
        return (request: Omit<ReqT, "command">): Promise<RespT> => this.request({ command, ...request });
    }

    private rpcWithProgress<ReqT extends CommandRequest, RespT extends CommandResponse>(command: ReqT["command"]) {
        return (request: Omit<ReqT, "command">, progressCallback?: ProgressCallback): Promise<RespT> =>
            this.request({ command, ...request }, progressCallback);
    }
}
