import type { ChatInputCommand, MessageCommand, CommandData } from 'commandkit';
import { InteractionContextType } from 'discord.js';

export const command: CommandData = {
  name: 'ping',
  description: "Comando para checar se estou online.",
  contexts: [InteractionContextType.Guild],
};

export const chatInput: ChatInputCommand = async ({ interaction, client }) => {
  const latency = (client.ws.ping ?? -1).toString();
  const response = `Pong! Latência: ${latency}ms`;

  await interaction.reply(response);
};

export const message: MessageCommand = async ({ message, client }) => {
  const latency = (client.ws.ping ?? -1).toString();
  const response = `Pong! Latência: ${latency}ms`;

  await message.reply(response);
};
