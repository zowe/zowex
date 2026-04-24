import type { ICommandDefinition } from "@zowe/imperative";

export const CopyDatasetOrMemberDefinition: ICommandDefinition = {
    handler: `${__dirname}/CopyDatasetOrMember.handler`,
    description: "Copy a data set or member",
    type: "command",
    name: "data-set",
    aliases: ["ds"],
    summary: "Copy a data set or member to a data set or member",
    examples: [
        {
            description: "Copy a data set or member",
            options: "'ibmuser.test.cntl",
        },
    ],
};
