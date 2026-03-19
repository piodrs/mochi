import type {
	ChatInputCommand,
	CommandData,
	CommandMetadata,
	MessageCommand,
} from "commandkit";
import { InteractionContextType } from "discord.js";
import { res } from "@/utils/res";

export const command: CommandData = {
	name: "ping",
	description: "Comando para checar se estou online.",
	contexts: [InteractionContextType.Guild],
};

export const metadata: CommandMetadata = {
	aliases: ["pong", "ws"],
};

export const chatInput: ChatInputCommand = async ({ interaction, client }) => {
	const latency = (client.ws.ping ?? -1).toString();
	const response = `${emojis.success} Pong! Latência: ${latency}ms`;

	await interaction.reply(res.success(response));
};

export const message: MessageCommand = async ({ message, client }) => {
	const latency = (client.ws.ping ?? -1).toString();
	const response = `${emojis.success} Pong! Latência: ${latency}ms`;

	await message.reply(res.success(response));
};
