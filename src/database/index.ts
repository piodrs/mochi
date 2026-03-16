import { env } from "@/env";
import { Logger } from "commandkit";
import mongoose from "mongoose";

try {
    await mongoose.connect(env.MONGO_URI, { dbName: env.DATABASE_NAME })
    Logger.info("🍃 Mongo connected succesfuly!")
} catch (error) {
    Logger.error(error);
    process.exit(1)
}