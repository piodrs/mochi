import "./database/index";
import "./constants";
import { commandkit } from "commandkit";
import { Client } from "discord.js";

const client = new Client({
	intents: ["Guilds", "GuildMembers", "GuildMessages", "MessageContent"],
});

commandkit.setPrefixResolver(async () => {
	return ["?", "-"];
});

export default client;
