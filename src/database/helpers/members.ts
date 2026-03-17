import type { GuildMember } from "discord.js";

import { type DatabaseSchema, db } from "@/database/schema";

type BumpSource = keyof typeof constants.guild.bumps;
type MemberData = DatabaseSchema["members"]["Data"];
type MemberBumps = NonNullable<NonNullable<MemberData["metrics"]>["bumps"]>;

function createMemberRecord(
	id: string,
	document: Awaited<ReturnType<typeof db.members.get>>,
) {
	return {
		id,
		document,
		data: document?.data,
	};
}

function buildMemberBumpMetrics(
	memberData: MemberData | undefined,
	source: BumpSource,
	bumpedAt: Date,
): MemberBumps {
	const currentBumps = memberData?.metrics?.bumps;
	const currentSourceCount = currentBumps?.bySource?.[source] ?? 0;

	return {
		...currentBumps,
		total: (currentBumps?.total ?? 0) + 1,
		bySource: {
			...currentBumps?.bySource,
			[source]: currentSourceCount + 1,
		},
		lastBumpedAt: bumpedAt,
	};
}

export async function getMemberRecord(member: GuildMember) {
	const id = db.members.id(member.id);
	const document = await db.members.get(id);

	return createMemberRecord(id, document);
}

export type AssignMemberData = DatabaseSchema["members"]["AssignArg"];
export async function upsetMemberRecord(
	member: GuildMember,
	data: AssignMemberData,
) {
	return await db.members.upset(db.members.id(member.id), data);
}

export type UpdateMemberData = DatabaseSchema["members"]["UpdateData"];
export async function updateMemberRecord(
	member: GuildMember,
	data: UpdateMemberData,
) {
	return await db.members.update(db.members.id(member.id), data);
}

export async function registerMemberBump(
	member: GuildMember,
	source: BumpSource,
	bumpedAt: Date,
) {
	const memberRecord = await getMemberRecord(member);

	return await upsetMemberRecord(member, {
		joinedAt: memberRecord.data?.joinedAt,
		updatedAt: bumpedAt,
		metrics: {
			...memberRecord.data?.metrics,
			bumps: buildMemberBumpMetrics(memberRecord.data, source, bumpedAt),
		},
	});
}
