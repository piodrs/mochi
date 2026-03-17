import { schema, type Typesaurus } from "typesaurus";

import type { GuildDocument } from "@/database/documents/guild.document";
import type { MemberDocument } from "@/database/documents/member.document";

export const db = schema(({ collection }) => ({
  guilds: collection<GuildDocument>(),
  members: collection<MemberDocument>(),
}));

export type DatabaseSchema = Typesaurus.Schema<typeof db>;
export type GuildSchema = DatabaseSchema["guilds"]["Data"];
export type MemberSchema = DatabaseSchema["members"]["Data"];
