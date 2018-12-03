
USE master
GO

IF EXISTS (SELECT name FROM master.dbo.sysdatabases WHERE name = N'WHJHAccountsDB')
DROP DATABASE [WHJHAccountsDB]
GO

IF EXISTS (SELECT name FROM master.dbo.sysdatabases WHERE name = N'WHJHGameMatchDB')
DROP DATABASE [WHJHGameMatchDB]
GO

IF EXISTS (SELECT name FROM master.dbo.sysdatabases WHERE name = N'WHJHGameScoreDB')
DROP DATABASE [WHJHGameScoreDB]
GO

IF EXISTS (SELECT name FROM master.dbo.sysdatabases WHERE name = N'WHJHPlatformDB')
DROP DATABASE [WHJHPlatformDB]
GO

IF EXISTS (SELECT name FROM master.dbo.sysdatabases WHERE name = N'WHJHRecordDB')
DROP DATABASE [WHJHRecordDB]
GO

IF EXISTS (SELECT name FROM master.dbo.sysdatabases WHERE name = N'WHJHTreasureDB')
DROP DATABASE [WHJHTreasureDB]
GO

IF EXISTS (SELECT name FROM master.dbo.sysdatabases WHERE name = N'WHJHEducateDB')
DROP DATABASE [WHJHEducateDB]
GO
