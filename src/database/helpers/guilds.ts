import type { Guild } from "discord.js";

import { type DatabaseSchema, db } from "@/database/schema";

type BumpSource = keyof typeof constants.guild.bumps;
type GuildData = DatabaseSchema["guilds"]["Data"];
type GuildBumpMap = NonNullable<GuildData["bumpState"]>;
type GuildBumpState = NonNullable<GuildBumpMap[BumpSource]>;

type DueGuildBumpReminder = {
	guildId: string;
	source: BumpSource;
	state: GuildBumpState;
};

function createGuildRecord(
	id: string,
	document: Awaited<ReturnType<typeof db.guilds.get>>,
) {
	return {
		id,
		document,
		data: document?.data,
	};
}

function mergeGuildBumpState(
	guildData: GuildData | undefined,
	source: BumpSource,
	bumpState: GuildBumpState,
): GuildBumpMap {
	return {
		...guildData?.bumpState,
		[source]: {
			...guildData?.bumpState?.[source],
			...bumpState,
		},
	};
}

function isReminderDue(state: GuildBumpState, now: Date) {
	if (!state.nextBumpAt) return false;
	if (state.nextBumpAt.getTime() > now.getTime()) return false;
	if (
		state.lastReminderSentAt &&
		state.lastReminderSentAt.getTime() >= state.nextBumpAt.getTime()
	)
		return false;

	return true;
}

export async function getGuildRecord(guild: Guild) {
	const id = db.guilds.id(guild.id);
	const document = await db.guilds.get(id);

	return createGuildRecord(id, document);
}

export type AssignGuildData = DatabaseSchema["guilds"]["AssignArg"];
export async function upsetGuildRecord(guild: Guild, data: AssignGuildData) {
	return await db.guilds.upset(db.guilds.id(guild.id), data);
}

export async function getGuildRecordById(guildId: string) {
	const id = db.guilds.id(guildId);
	const document = await db.guilds.get(id);

	return createGuildRecord(id, document);
}

export async function saveGuildBumpState(
	guild: Guild,
	source: BumpSource,
	now: Date,
	nextBumpAt: Date,
) {
	const guildRecord = await getGuildRecord(guild);

	return await upsetGuildRecord(guild, {
		updatedAt: now,
		bumpState: mergeGuildBumpState(guildRecord.data, source, {
			lastBumpedAt: now,
			nextBumpAt,
			lastReminderSentAt: undefined,
		}),
	});
}

export async function markGuildBumpReminderSentById(
	guildId: string,
	source: BumpSource,
	sentAt = new Date(),
) {
	const guildRecord = await getGuildRecordById(guildId);

	return await db.guilds.upset(db.guilds.id(guildId), {
		updatedAt: sentAt,
		bumpState: mergeGuildBumpState(guildRecord.data, source, {
			lastReminderSentAt: sentAt,
		}),
	});
}

export async function getDueGuildBumpReminders(now: Date) {
	const documents = await db.guilds.all();
	const dueReminders: DueGuildBumpReminder[] = [];

	for (const document of documents) {
		const bumpState = document.data.bumpState;

		if (!bumpState) continue;

		const entries = Object.entries(bumpState) as [
			BumpSource,
			GuildBumpState | undefined,
		][];

		for (const [source, state] of entries) {
			if (!state || !isReminderDue(state, now)) continue;

			dueReminders.push({
				guildId: document.ref.id,
				source,
				state,
			});
		}
	}

	return dueReminders;
}
