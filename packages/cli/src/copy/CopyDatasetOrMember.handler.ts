import type { IHandlerParameters } from "@zowe/imperative";
import type { ds, ZSshClient } from "@zowe/zowex-for-zowe-sdk";
import { SshBaseHandler } from "../SshBaseHandler";

export default class CopyDatasetOrMemberHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<ds.CopyDatasetResponse> {
        const fromDataset = params.arguments.fromDataset as string;
        const toDataset = params.arguments.toDataset as string;
        const replace = params.arguments?.replace as boolean;
        const overwrite = params.arguments?.overwrite as boolean;

        try {
            const response = await client.ds.copyDatasetOrMember({
                source: fromDataset,
                target: toDataset,
                replace,
                overwrite,
            });

            if (response.success) {
                let message = `Successfully copied '${fromDataset}' to '${toDataset}'.`;

                if (response.targetCreated) {
                    message = `New data set '${toDataset}' created and copied from '${fromDataset}'.`;
                } else if (response.memberCreated) {
                    message = `New member '${toDataset}' created and copied from '${fromDataset}'.`;
                } else if (response.overwrite) {
                    message = `Target data set '${toDataset}' was overwritten with contents of '${fromDataset}'.`;
                } else if (response.replace) {
                    message = `Target '${toDataset}' was updated/replaced with contents of '${fromDataset}'.`;
                }

                params.response.console.log(message);
                params.response.data.setObj(response);
            }
            return response;
        } catch (err) {
            params.response.console.error(err.message);
            throw err;
        }
    }
}
