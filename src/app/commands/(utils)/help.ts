import type {
	ChatInputCommand,
	CommandData,
	CommandMetadata,
	MessageCommand,
} from "commandkit";
import { InteractionContextType, inlineCode } from "discord.js";
import { res } from "@/utils/res";

export const command: CommandData = {
	name: "help",
	description: "Mostra todos os meus comandos disponíveis.",
	contexts: [InteractionContextType.Guild],
};

export const metadata: CommandMetadata = {
	aliases: ["ajuda", "h"],
};

type CommandKitInstance = Parameters<ChatInputCommand>[0]["commandkit"];
type RegisteredCommand = ReturnType<
	CommandKitInstance["commandHandler"]["registrar"]["getCommandsData"]
>[number];

function formatAliases(aliases: string[] | undefined): string {
	if (!aliases?.length) return "";
	return ` (${aliases.map(inlineCode).join(", ")})`;
}

function formatCommand({
	name,
	description,
	__metadata,
}: RegisteredCommand): string {
	if (!description)
		throw new Error(`Command "${name}" is missing description.`);
	return `**/${name}** - ${inlineCode(description)}${formatAliases(__metadata?.aliases)}`;
}

function buildHelpContent(commandkit: CommandKitInstance): string {
	return commandkit.commandHandler.registrar
		.getCommandsData()
		.filter((cmd) => cmd.name !== "help" && cmd.name !== "emit")
		.sort((a, b) => a.name.localeCompare(b.name))
		.map(formatCommand)
		.join("\n");
}

export const chatInput: ChatInputCommand = async ({
	interaction,
	commandkit,
}) => {
	await interaction.reply(res.info(buildHelpContent(commandkit)));
};

export const message: MessageCommand = async ({ message, commandkit }) => {
	await message.reply(res.info(buildHelpContent(commandkit)));
};
