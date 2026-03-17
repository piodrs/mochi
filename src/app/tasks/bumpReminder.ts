import { task } from "@commandkit/tasks";
import { roleMention } from "discord.js";
import {
	getDueGuildBumpReminders,
	markGuildBumpReminderSentById,
} from "@/database/helpers/guilds";
import { res } from "@/utils/res";

const { guild } = constants;

export default task({
	name: "bumpReminder",
	schedule: "*/10 * * * *",
	async execute({ client }) {
		const dueReminders = await getDueGuildBumpReminders(new Date());

		if (!dueReminders.length) return;

		const channel = await client.channels.fetch(guild.channels.bump);

		if (!channel?.isTextBased() || !("send" in channel)) return;

		for (const reminder of dueReminders) {
			const state = reminder.state;

			if (!state.nextBumpAt) continue;

			const service = guild.bumps[reminder.source];

			await channel.send(
				res.info(
					`${roleMention(guild.roles.pingBump)} ${emojis.next} O bump do ${service.label} já pode ser usado novamente.`,
				),
			);

			await markGuildBumpReminderSentById(reminder.guildId, reminder.source);
		}
	},
});
