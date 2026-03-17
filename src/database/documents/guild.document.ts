export interface GuildDocument {
	updatedAt?: Date;
	bumpState?: {
		disboard?: {
			lastBumpedAt?: Date;
			nextBumpAt?: Date;
			lastReminderSentAt?: Date;
		};
		discadia?: {
			lastBumpedAt?: Date;
			nextBumpAt?: Date;
			lastReminderSentAt?: Date;
		};
	};
}
