USE [WHJHAccountsDB]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_BindSpreadCode]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_BindSpreadCode]
GO

SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO


CREATE PROC [dbo].[GSP_MB_BindSpreadCode]
  @dwUserID INT,
  @strCode NCHAR(8),
  @strErrorDescribe NVARCHAR(127) OUTPUT  -- 输出信息
-- WITH ENCRYPTION
AS

-- 属性设置
SET NOCOUNT ON

-- 执行逻辑
BEGIN
	DECLARE @tgSpreadID	INT
	DECLARE @tgRebate SMALLINT
	DECLARE @tgUserType TINYINT
	DECLARE @tgState TINYINT
	SELECT @tgSpreadID=UserID, @tgRebate=Rebate, @tgUserType=UserType, @tgState=State
	FROM SpreadCode(NOLOCK) WHERE Code=@strCode
	IF @tgSpreadID IS NULL OR @tgState=0
	BEGIN
		SET @strErrorDescribe=N'推广码不存在!'
		RETURN 1
	END
	DECLARE @SpreadID INT
	DECLARE @Hierarchy INT
	DECLARE @spRebate SMALLINT
	SELECT @SpreadID=SpreaderID, @Hierarchy=Hierarchy, @spRebate=Rebate FROM AccountsInfo(NOLOCK)
	WHERE UserID=@tgSpreadID
	IF @SpreadID IS NULL
	BEGIN
		SET @strErrorDescribe=N'推广用户不存在!'
		RETURN 1
	END
	IF @tgRebate > @spRebate
	BEGIN
		SET @strErrorDescribe=N'推广返点设置错误!'
		RETURN 1
	END

	SELECT @SpreadID=SpreaderID FROM AccountsInfo(NOLOCK)
	WHERE UserID=@dwUserID
	IF @SpreadID IS NULL
	BEGIN
		SET @strErrorDescribe=N'账号不存在!'
		RETURN 1
	END
	IF @SpreadID <> 0
	BEGIN
		SET @strErrorDescribe=N'账号已绑定上级!'
		RETURN 1
	END
	-- 绑定
	UPDATE AccountsInfo SET SpreaderID=@tgSpreadID, Rebate=@tgRebate, UserType=@tgUserType, Hierarchy=@Hierarchy+1
	WHERE UserID=@dwUserID
	UPDATE SpreadCode SET Registed=Registed+1
	WHERE Code=@strCode

	SELECT @tgSpreadID AS SpreaderID, @tgRebate AS Rebate, @tgUserType AS UserType
END

RETURN 0

GO


