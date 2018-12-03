USE [WHJHRecordDB]
GO

SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE TABLE [dbo].[RecordDailyReport](
	[Day] [char](8) NOT NULL,
	[UserID] [int] NOT NULL,
	[Ctime] [bigint] NOT NULL,
	[GameID] [int] NOT NULL,
	[pSales] [bigint] NOT NULL,
	[pTaxes] [bigint] NOT NULL,
	[pPresent] [bigint] NOT NULL,
	[pRebate] [bigint] NOT NULL,
	[pCharge] [bigint] NOT NULL,
	[pWithdraw] [bigint] NOT NULL,
	[gSales] [bigint] NOT NULL,
	[gTaxes] [bigint] NOT NULL,
	[gPresent] [bigint] NOT NULL,
	[gRebate] [bigint] NOT NULL,
	[gCharge] [bigint] NOT NULL,
	[gWithdraw] [bigint] NOT NULL,
	[Registed] [int] NOT NULL,
	[Binded] [int] NOT NULL,
	[Played] [int] NOT NULL,
	[Infers] [int] NOT NULL,
	[Balance] [bigint] NOT NULL,
	[FirstCharge] [int] NOT NULL,
	[pRbCount] [int] NOT NULL,
	[pRbMen] [int] NOT NULL,
	[gRbCount] [int] NOT NULL,
	[gRbMen] [int] NOT NULL,
 CONSTRAINT [PK_RecordDailyReport] PRIMARY KEY CLUSTERED 
(
	[Day] ASC,
	[UserID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]
GO

ALTER TABLE [dbo].[RecordDailyReport] ADD  DEFAULT ((0)) FOR [pRbCount]
GO

ALTER TABLE [dbo].[RecordDailyReport] ADD  DEFAULT ((0)) FOR [pRbMen]
GO

ALTER TABLE [dbo].[RecordDailyReport] ADD  DEFAULT ((0)) FOR [gRbCount]
GO

ALTER TABLE [dbo].[RecordDailyReport] ADD  DEFAULT ((0)) FOR [gRbMen]
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'报表日' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'Day'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'用户ID' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'UserID'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'报表时间' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'Ctime'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'用户游戏ID' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'GameID'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'个人直销收入' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'pSales'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'个人税收' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'pTaxes'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'个人活动礼金' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'pPresent'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'个人返佣' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'pRebate'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'个人上分' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'pCharge'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'个人下分' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'pWithdraw'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'团队直销收入' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'gSales'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'团队税收' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'gTaxes'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'团队活动礼金' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'gPresent'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'团队返佣' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'gRebate'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'团队上分' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'gCharge'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'团队下分' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'gWithdraw'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'注册人数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'Registed'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'绑定人数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'Binded'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'游戏人数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'Played'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'下级人数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'Infers'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'团队游戏币' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'Balance'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'首充人数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'FirstCharge'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'个人返佣笔数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'pRbCount'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'个人返佣人数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'pRbMen'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'团队返佣笔数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'gRbCount'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'团队返佣人数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'RecordDailyReport', @level2type=N'COLUMN',@level2name=N'gRbMen'
GO


