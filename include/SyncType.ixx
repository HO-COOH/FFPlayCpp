export module ffplay:SyncType;

export enum class SyncType
{
	AudioMaster, /* default choice */
	VideoMaster,
	External, /* synchronize to an external clock */
};