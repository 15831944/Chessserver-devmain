USE [WHJHAccountsDB]
GO

SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE TABLE [dbo].[SpreadCode](
	[Code] [nchar](8) NOT NULL,
	[UserID] [int] NOT NULL,
	[Rebate] [smallint] NOT NULL,
	[UserType] [tinyint] NOT NULL,
	[Registed] [int] NOT NULL,
	[Ctime] [datetime] NOT NULL,
	[State] [tinyint] NOT NULL,
 CONSTRAINT [PK_SpreadCode] PRIMARY KEY CLUSTERED 
(
	[Code] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]
GO

ALTER TABLE [dbo].[SpreadCode] ADD  CONSTRAINT [DF_SpreadCode_UserID]  DEFAULT ((0)) FOR [UserID]
GO

ALTER TABLE [dbo].[SpreadCode] ADD  CONSTRAINT [DF_SpreadCode_Rebate]  DEFAULT ((0)) FOR [Rebate]
GO

ALTER TABLE [dbo].[SpreadCode] ADD  CONSTRAINT [DF_SpreadCode_UserType]  DEFAULT ((1)) FOR [UserType]
GO

ALTER TABLE [dbo].[SpreadCode] ADD  CONSTRAINT [DF_SpreadCode_Registed]  DEFAULT ((0)) FOR [Registed]
GO

ALTER TABLE [dbo].[SpreadCode] ADD  CONSTRAINT [DF_SpreadCode_Ctime]  DEFAULT (getdate()) FOR [Ctime]
GO

ALTER TABLE [dbo].[SpreadCode] ADD  CONSTRAINT [DF_SpreadCode_State]  DEFAULT ((0)) FOR [State]
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'推广码' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'SpreadCode', @level2type=N'COLUMN',@level2name=N'Code'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'代理UID' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'SpreadCode', @level2type=N'COLUMN',@level2name=N'UserID'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'下级返点' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'SpreadCode', @level2type=N'COLUMN',@level2name=N'Rebate'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'下级账户类型' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'SpreadCode', @level2type=N'COLUMN',@level2name=N'UserType'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'已推广人数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'SpreadCode', @level2type=N'COLUMN',@level2name=N'Registed'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'创建时间' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'SpreadCode', @level2type=N'COLUMN',@level2name=N'Ctime'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'状态0=删除' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'SpreadCode', @level2type=N'COLUMN',@level2name=N'State'
GO


