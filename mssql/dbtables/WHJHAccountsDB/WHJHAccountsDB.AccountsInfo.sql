USE [WHJHAccountsDB]
GO

SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE TABLE [dbo].[AccountsInfo](
  [UserID] [int] IDENTITY(1,1) NOT NULL,
  [GameID] [int] NOT NULL,
  [SpreaderID] [int] NOT NULL,
  [Accounts] [nvarchar](31) NOT NULL,
  [NickName] [nvarchar](31) NOT NULL,
  [RegAccounts] [nvarchar](31) NOT NULL,
  [UnderWrite] [nvarchar](250) NOT NULL,
  [PassPortID] [nvarchar](18) NOT NULL,
  [Compellation] [nvarchar](16) NOT NULL,
  [LogonPass] [nchar](32) NOT NULL,
  [InsurePass] [nchar](32) NOT NULL,
  [DynamicPass] [nchar](32) NOT NULL,
  [DynamicPassTime] [datetime] NOT NULL,
  [FaceID] [smallint] NOT NULL,
  [CustomID] [int] NOT NULL,
  [UserRight] [int] NOT NULL,
  [MasterRight] [int] NOT NULL,
  [ServiceRight] [int] NOT NULL,
  [MasterOrder] [tinyint] NOT NULL,
  [MemberOrder] [tinyint] NOT NULL,
  [MemberOverDate] [datetime] NOT NULL,
  [MemberSwitchDate] [datetime] NOT NULL,
  [CustomFaceVer] [tinyint] NOT NULL,
  [Gender] [tinyint] NOT NULL,
  [Nullity] [tinyint] NOT NULL,
  [NullityOverDate] [datetime] NOT NULL,
  [StunDown] [tinyint] NOT NULL,
  [MoorMachine] [tinyint] NOT NULL,
  [IsAndroid] [tinyint] NOT NULL,
  [WebLogonTimes] [int] NOT NULL,
  [GameLogonTimes] [int] NOT NULL,
  [PlayTimeCount] [int] NOT NULL,
  [OnLineTimeCount] [int] NOT NULL,
  [LastLogonIP] [nvarchar](15) NOT NULL,
  [LastLogonDate] [datetime] NOT NULL,
  [LastLogonMobile] [nvarchar](11) NOT NULL,
  [LastLogonMachine] [nvarchar](32) NOT NULL,
  [RegisterIP] [nvarchar](15) NOT NULL,
  [RegisterDate] [datetime] NOT NULL,
  [RegisterMobile] [nvarchar](11) NOT NULL,
  [RegisterMachine] [nvarchar](32) NOT NULL,
  [RegisterOrigin] [tinyint] NULL,
  [PlatformID] [smallint] NOT NULL,
  [UserUin] [nvarchar](32) NOT NULL,
  [RankID] [int] NULL,
  [AgentID] [int] NOT NULL,
  [PlaceName] [nvarchar](33) NOT NULL,
  [LevelId] [int] NOT NULL,
  [RechargeTimes] [int] NOT NULL,
  [OneRechargeDate] [int] NOT NULL,
  [UserType] [tinyint] NOT NULL,
  [ParentAgent] [int] NOT NULL,
  [AgentMembership] [int] NOT NULL,
  [WithdrawTime] [int] NOT NULL,
  [Rebate] [smallint] NULL,
  [Hierarchy] [int] NOT NULL,
 CONSTRAINT [PK_AccountsInfo_UserID] PRIMARY KEY CLUSTERED 
(
  [UserID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_GameID]  DEFAULT ((0)) FOR [GameID]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_SpreaderID]  DEFAULT ((0)) FOR [SpreaderID]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_Nickname]  DEFAULT ('') FOR [NickName]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_UserAccounts_UnderWrite]  DEFAULT ('') FOR [UnderWrite]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_PassPortID]  DEFAULT (N'') FOR [PassPortID]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_Compellation]  DEFAULT (N'') FOR [Compellation]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_InsurePass]  DEFAULT (N'') FOR [InsurePass]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_DynamicPass]  DEFAULT ('') FOR [DynamicPass]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_DynamicPassTime]  DEFAULT (getdate()) FOR [DynamicPassTime]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_UserAccounts_FaceID]  DEFAULT ((0)) FOR [FaceID]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_CustomFaceID]  DEFAULT ((0)) FOR [CustomID]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_UserRight]  DEFAULT ((0)) FOR [UserRight]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_MasterRight]  DEFAULT ((0)) FOR [MasterRight]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_ServiceRight]  DEFAULT ((0)) FOR [ServiceRight]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_MasterOrder]  DEFAULT ((0)) FOR [MasterOrder]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_UserAccounts_MemberOrder]  DEFAULT ((0)) FOR [MemberOrder]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_UserAccounts_MemberOverDate]  DEFAULT (((1980)-(1))-(1)) FOR [MemberOverDate]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_MemberSwitchDate]  DEFAULT (((1980)-(1))-(1)) FOR [MemberSwitchDate]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_CustomFaceVer]  DEFAULT ((0)) FOR [CustomFaceVer]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_Gender]  DEFAULT ((0)) FOR [Gender]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_UserAccounts_ServiceNullity]  DEFAULT ((0)) FOR [Nullity]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_NullityOverDate]  DEFAULT ('1900-01-01') FOR [NullityOverDate]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_CloseDown]  DEFAULT ((0)) FOR [StunDown]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_MoorMachine]  DEFAULT ((0)) FOR [MoorMachine]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_IsAndroid]  DEFAULT ((0)) FOR [IsAndroid]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_WebLogonTimes]  DEFAULT ((0)) FOR [WebLogonTimes]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_UserAccounts_AllLogonTimes]  DEFAULT ((0)) FOR [GameLogonTimes]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_PlayTimeCount]  DEFAULT ((0)) FOR [PlayTimeCount]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_OnLineTimeCount]  DEFAULT ((0)) FOR [OnLineTimeCount]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_UserAccounts_LastLogonDate]  DEFAULT (getdate()) FOR [LastLogonDate]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_RegisterMobile1]  DEFAULT (N'') FOR [LastLogonMobile]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_MachineSerial]  DEFAULT ('------------') FOR [LastLogonMachine]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_UserAccounts_RegisterDate]  DEFAULT (getdate()) FOR [RegisterDate]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_RegisterMobile]  DEFAULT (N'') FOR [RegisterMobile]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_RegisterMachine]  DEFAULT (N'------------') FOR [RegisterMachine]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_RegisterOrigin]  DEFAULT ((0)) FOR [RegisterOrigin]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_UserUin]  DEFAULT ((0)) FOR [PlatformID]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_UserUin_1]  DEFAULT (N'') FOR [UserUin]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_AgentID]  DEFAULT ((0)) FOR [AgentID]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_PlaceName]  DEFAULT (N'') FOR [PlaceName]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_LevelId]  DEFAULT ((1)) FOR [LevelId]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF__AccountsI__Recha__7A3223E8]  DEFAULT ((0)) FOR [RechargeTimes]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_OneRechargeDate]  DEFAULT ((0)) FOR [OneRechargeDate]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_UserType]  DEFAULT ((1)) FOR [UserType]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_ParentAgent]  DEFAULT ((0)) FOR [ParentAgent]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_AgentMembership]  DEFAULT ((0)) FOR [AgentMembership]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_WithdrawTime]  DEFAULT ((0)) FOR [WithdrawTime]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  CONSTRAINT [DF_AccountsInfo_Rebate]  DEFAULT ((0)) FOR [Rebate]
GO

ALTER TABLE [dbo].[AccountsInfo] ADD  DEFAULT ((0)) FOR [Hierarchy]
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'用户标识' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'UserID'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'游戏标识' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'GameID'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'推广员标识' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'SpreaderID'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'用户帐号' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'Accounts'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'用户昵称' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'NickName'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'注册帐号' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'RegAccounts'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'个性签名' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'UnderWrite'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'身份证号' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'PassPortID'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'真实名字' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'Compellation'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'登录密码' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'LogonPass'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'安全密码' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'InsurePass'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'动态密码' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'DynamicPass'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'动态密码更新时间' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'DynamicPassTime'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'头像标识' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'FaceID'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'自定标识' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'CustomID'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'用户权限' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'UserRight'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'管理权限' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'MasterRight'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'服务权限' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'ServiceRight'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'管理等级' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'MasterOrder'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'会员等级' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'MemberOrder'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'过期日期' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'MemberOverDate'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'切换时间' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'MemberSwitchDate'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'头像版本' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'CustomFaceVer'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'用户性别' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'Gender'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'禁止服务' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'Nullity'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'禁止时间' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'NullityOverDate'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'关闭标志' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'StunDown'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'固定机器' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'MoorMachine'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'是否机器人' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'IsAndroid'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'登录次数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'WebLogonTimes'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'登录次数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'GameLogonTimes'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'游戏时间' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'PlayTimeCount'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'在线时间' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'OnLineTimeCount'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'登录地址' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'LastLogonIP'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'登录时间' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'LastLogonDate'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'登录手机' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'LastLogonMobile'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'登录机器' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'LastLogonMachine'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'注册地址' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'RegisterIP'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'注册时间' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'RegisterDate'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'注册手机' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'RegisterMobile'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'注册机器' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'RegisterMachine'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'PC       0x00     ,
ANDROID  0x10(cocos 0x11,u3d 0x12)     ,
ITOUCH   0x20     ,
IPHONE   0x30(cocos 0x31,u3d 0x32)     ,
IPAD     0x40(cocos 0x41,u3d 0x42)     ,
WEB      0x50     ' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'RegisterOrigin'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'地名' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'PlaceName'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'会员所属层级' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'LevelId'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'充值次数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'RechargeTimes'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'首次充值时间' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'OneRechargeDate'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'用户类型(1会员2代理)' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'UserType'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N' 会员所属代理ID ' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'ParentAgent'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'代理下级会员数量' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'AgentMembership'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'下分次数' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'WithdrawTime'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'用户返点' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'Rebate'
GO

EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'代理层级' , @level0type=N'SCHEMA',@level0name=N'dbo', @level1type=N'TABLE',@level1name=N'AccountsInfo', @level2type=N'COLUMN',@level2name=N'Hierarchy'
GO


