import type { GuildMember } from "discord.js";

import { db, type DatabaseSchema } from "@/database/schema";

export async function getMemberRecord(member: GuildMember) {
  const id = db.members.id(member.id);
  const document = await db.members.get(id);

  return {
    id,
    document,
    data: document?.data,
  };
}

export type AssignMemberData = DatabaseSchema["members"]["AssignArg"];
export async function upsetMemberRecord(member: GuildMember, data: AssignMemberData) {
  return await db.members.upset(db.members.id(member.id), data);
}

export type UpdateMemberData = DatabaseSchema["members"]["UpdateData"];
export async function updateMemberRecord(member: GuildMember, data: UpdateMemberData) {
  return await db.members.update(db.members.id(member.id), data);
}
