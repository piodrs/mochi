import type { EventHandler } from "commandkit";
import { time } from "discord.js";
import { saveGuildBumpState } from "@/database/helpers/guilds";
import { registerMemberBump } from "@/database/helpers/members";
import { res } from "@/utils/res";

const { guild } = constants;

type BumpSource = keyof typeof guild.bumps;

function getBumpSource(authorId: string): BumpSource | null {
	const entries = Object.entries(guild.bumps) as [
		BumpSource,
		(typeof guild.bumps)[BumpSource],
	][];

	for (const [source, config] of entries)
		if (config.botId && config.botId === authorId) return source;

	return null;
}

function isDisboardBumpMessage(
	message: Parameters<EventHandler<"messageCreate">>[0],
): boolean {
	const embed = message.embeds[0];

	if (!embed) return false;

	const description = embed.description?.toLowerCase() ?? "";

	return description.includes("bump done");
}

function isDiscadiaBumpMessage(
	message: Parameters<EventHandler<"messageCreate">>[0],
): boolean {
	return message.content.toLowerCase().includes("has been successfully bumped");
}

function isValidBumpConfirmation(
	source: BumpSource,
	message: Parameters<EventHandler<"messageCreate">>[0],
): boolean {
	switch (source) {
		case "disboard":
			return isDisboardBumpMessage(message);
		case "discadia":
			return isDiscadiaBumpMessage(message);
	}
}

const handler: EventHandler<"messageCreate"> = async (message) => {
	if (!message.guild) return;
	if (message.channelId !== guild.channels.bump) return;

	const source = getBumpSource(message.author.id);

	if (!source) return;
	if (!isValidBumpConfirmation(source, message)) return;

	const now = new Date();
	const config = guild.bumps[source];
	const nextBumpAt = new Date(
		now.getTime() + config.cooldownHours * 60 * 60 * 1000,
	);
	await saveGuildBumpState(message.guild, source, now, nextBumpAt);

	const bumperId = message.interactionMetadata?.user.id;
	if (!bumperId) return;

	const member = await message.guild.members.fetch(bumperId).catch(() => null);
	if (!member) return;

	await registerMemberBump(member, source, now);

	await message.channel.send(
		res.success(
			`${emojis.keqingHeart} Obrigada pelo bump, ${member}. O próximo bump do **${config.label}** poderá ser usado ${time(Math.floor(nextBumpAt.getTime() / 1000), "R")}.`,
		),
	);
};

export default handler;
