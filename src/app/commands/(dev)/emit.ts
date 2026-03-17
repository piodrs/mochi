import type { ChatInputCommand, CommandData } from "commandkit";
import {
	ApplicationCommandOptionType,
	InteractionContextType,
	PermissionFlagsBits,
} from "discord.js";
import { res } from "@/utils/res";

export const command: CommandData = {
	name: "emit",
	description: "Simula eventos para desenvolvimento.",
	contexts: [InteractionContextType.Guild],
	default_member_permissions: PermissionFlagsBits.Administrator.toString(),
	options: [
		{
			name: "join",
			description: "Simula a entrada de um membro.",
			type: ApplicationCommandOptionType.Subcommand,
		},
	],
};

export const chatInput: ChatInputCommand = async ({ interaction, client }) => {
	const { guild, user, options } = interaction;

	if (!guild) {
		await interaction.reply(
			res.error("Este comando só pode ser usado em um servidor.", true),
		);
		return;
	}

	const member = await guild.members.fetch(user.id);

	switch (options.getSubcommand(true)) {
		case "join":
			client.emit("guildMemberAdd", member);
			await interaction.reply(
				res.success(
					`${emojis.success} Simulei o evento **join** para ${member}.`,
					true,
				),
			);
			break;
	}
};
