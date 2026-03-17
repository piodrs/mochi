import { schema, type Typesaurus } from "typesaurus";

import type { MemberDocument } from "@/database/documents/member.document";

export const db = schema(({ collection }) => ({
  members: collection<MemberDocument>(),
}));

export type DatabaseSchema = Typesaurus.Schema<typeof db>;
export type MemberSchema = DatabaseSchema["members"]["Data"];
