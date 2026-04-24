import type { IHandlerParameters } from "@zowe/imperative";
import { SshBaseHandler } from "../../SshBaseHandler";
import type { ds, ZSshClient } from "@zowe/zowex-for-zowe-sdk";
import { CopyDatasetRequest } from "../CopyDatasetRequest";

export default class CopyDatasetOrMemberHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<ds.CopyDatasetResponse> {
        const fromDataset = params.arguments.fromDataset as string;
        const toDataset = params.arguments.toDataset as string;
        const replace = params.arguments.replace as boolean;
        const overwrite = params.arguments.overwrite as boolean;

        const request: CopyDatasetRequest = {
            source: fromDataset,
            target: toDataset,
            replace: replace,
            overwrite: overwrite,
        };

        try {
            const response = await client.ds.copy({});

            if (response.success) {
                const result = response.result;
                let message = `Successfully copied '${fromDataset}' to '${toDataset}'.`;

                if (result.targetCreated) {
                    message = `New data set '${toDataset}' created and copied from '${fromDataset}'.`;
                } else if (result.memberCreated) {
                    message = `New member '${toDataset}' created and copied from '${fromDataset}'.`;
                } else if (result.overwritten) {
                    message = `Target data set '${toDataset}' was overwritten with contents of '${fromDataset}'.`;
                } else if (result.replaced) {
                    message = `Target '${toDataset}' was updated/replaced with contents of '${fromDataset}'.`;
                }

                params.response.console.log(message);
                params.response.data.setObj(result);
            }
        } catch (err) {
            params.response.console.error(err.message);
            throw err;
        }
    }
}
