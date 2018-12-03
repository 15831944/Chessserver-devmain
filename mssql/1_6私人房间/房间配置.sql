USE WHJHPlatformDB
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_WritePersonalFeeParameter]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_WritePersonalFeeParameter]
--ɾ������
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_DeletePersonalFeeParameter]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_DeletePersonalFeeParameter]

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_GetPersonalParameter]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_GetPersonalParameter]

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_GetPersonalFeeParameter]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_GetPersonalFeeParameter]

---����˽�˷������
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_LoadPersonalRoomParameter]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_LoadPersonalRoomParameter]

---����˽�˷������
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_LoadPersonalRoomParameterByKindID]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].GSP_GS_LoadPersonalRoomParameterByKindID
GO

----��ȡ������ҽ�����ķ���
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_GetPersonalRoomUserScore]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_GetPersonalRoomUserScore]
GO


----д�����ʱ��
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GS_CloseRoomWriteDissumeTime]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GS_CloseRoomWriteDissumeTime]
GO

----��ȡ˽�˷���������
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_GetPersonalRule]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_GetPersonalRule]
GO

SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

------------------------------------------------------
---��������
CREATE PROC GSP_GS_WritePersonalFeeParameter
	@KindID	INT,							-- ��������
	@DrawCountLimit	INT,					-- ����
	@DrawTimeLimit	INT,					-- ��ҵ��߶೤ʱ����ɢ����
	@TableFee	INT,						-- ���䴴���೤ʱ���δ��ʼ��Ϸ��ɢ����
	@AAPayFee   INT,						-- AA�Ʒ���
	@IniScore	INT,						-- ���䴴����೤ʱ���δ��ʼ��Ϸ��ɢ����
	@cbGameMode TINYINT,					-- ��Ϸģʽ
	@strErrorDescribe NVARCHAR(127) OUTPUT	-- �����Ϣ
WITH ENCRYPTION AS

BEGIN
	INSERT INTO PersonalTableFee (KindID, DrawCountLimit,DrawTimeLimit, TableFee,AAPayFee,IniScore,GameMode) 
	VALUES (@KindID, @DrawCountLimit,@DrawTimeLimit, @TableFee,@AAPayFee, @IniScore,@cbGameMode) 
END

return 0
GO

--ɾ������
CREATE PROC GSP_GS_DeletePersonalFeeParameter
	@KindID	INT,				-- 
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

BEGIN
	
	DELETE  FROM PersonalTableFee WHERE  KindID = @KindID
	
END

return 0
GO


------------------------------------------------------
---��������
CREATE PROC GSP_MB_GetPersonalFeeParameter
	@dwKindID INT,								-- ��Ϸ I D
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

BEGIN

	SELECT  DrawCountLimit, DrawTimeLimit, TableFee,AAPayFee,IniScore,GameMode FROM PersonalTableFee WHERE KindID=@dwKindID	
END

return 0
GO

---------------------------------------------------------------
-- ��������
CREATE PROC GSP_GS_LoadPersonalRoomParameter
	@dwKindID INT,								-- �����ʶ
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON	
	DECLARE @cbIsJoinGame TINYINT						-- �Ƿ������Ϸ
	DECLARE @cbMinPeople TINYINT						-- ������Ϸ����С����
	DECLARE @cbMaxPeople TINYINT						-- ������Ϸ���������

	-- ��������
	DECLARE @lPersonalRoomTax  BIGINT					-- ˰��
	DECLARE @lMaxCellScore BIGINT						-- ���׷�
	DECLARE @wCanCreateCount int						-- ���Դ�������󷿼���Ŀ
	DECLARE @wPlayTurnCount INT							-- �����ܹ�������Ϸ��������
	DECLARE @cbCreateRight TINYINT						-- ����˽�˷���Ȩ��

	-- ��������
	DECLARE @wPlayTimeLimit	INT						-- �����ܹ�������Ϸ�����ʱ��
	DECLARE @wTimeAfterBeginCount	INT				-- һ����Ϸ��ʼ��೤ʱ����ɢ����
	DECLARE @wTimeOffLineCount	INT					-- ��ҵ��߶೤ʱ����ɢ����
	DECLARE @wTimeNotBeginGame	INT					-- ���䴴���೤ʱ���δ��ʼ��Ϸ��ɢ����
	DECLARE @wTimeNotBeginAfterCreateRoom	INT		-- ���䴴����೤ʱ���δ��ʼ��Ϸ��ɢ����

	DECLARE @wBeginFreeTime		INT					-- ���ʱ��-��ʼ
	DECLARE @wEndFreeTime		INT					 -- ���ʱ��-����
-- ִ���߼�
BEGIN		
	SET @cbIsJoinGame = 0
	SET @cbMinPeople = 0
	SET @cbMaxPeople = 0	
	SET @lMaxCellScore = 0
	SET @lPersonalRoomTax = 0
	SET @lMaxCellScore = 0
	SET @wCanCreateCount = 0
	SET @wPlayTurnCount = 0
	SET @wPlayTimeLimit = 0
	SET @wTimeAfterBeginCount = 0
	SET @wTimeOffLineCount = 0
	SET @wTimeNotBeginGame = 0
	SET @wTimeNotBeginAfterCreateRoom = 0
	SET @cbCreateRight = 0
	SET @wBeginFreeTime = 0
	SET @wEndFreeTime = 0

	----��ѯ���һ����ͬ������Ϸ�޸�����
	--DECLARE @dwLastServerID INT
	--SET @dwLastServerID = 0
	--SELECT @dwLastServerID=KindID FROM GameRoomInfo WHERE KindID = @wServerID

	-- ���ط���
	SELECT @cbIsJoinGame=IsJoinGame,@cbMinPeople=MinPeople, @cbMaxPeople=MaxPeople, @lPersonalRoomTax =PersonalRoomTax,
	@lMaxCellScore= MaxCellScore, @wCanCreateCount= CanCreateCount, @wPlayTurnCount=PlayTurnCount, @wPlayTimeLimit= PlayTimeLimit, @wTimeAfterBeginCount = TimeAfterBeginCount, 
	@wTimeOffLineCount = TimeOffLineCount, @wTimeNotBeginGame = TimeNotBeginGame, @wTimeNotBeginAfterCreateRoom = TimeNotBeginAfterCreateRoom, @cbCreateRight = CreateRight,
	@wBeginFreeTime = BeginFreeTime,@wEndFreeTime = EndFreeTime
	FROM PersonalRoomInfo WHERE KindID = @dwKindID



	SELECT @cbIsJoinGame AS IsJoinGame,@cbMinPeople AS MinPeople, @cbMaxPeople AS MaxPeople, @lPersonalRoomTax AS PersonalRoomTax,
	@lMaxCellScore AS MaxCellScore, @wCanCreateCount AS CanCreateCount, @wPlayTurnCount AS PlayTurnCount, @wPlayTimeLimit AS PlayTimeLimit, @wTimeAfterBeginCount AS TimeAfterBeginCount, 
	@wTimeOffLineCount AS TimeOffLineCount, @wTimeNotBeginGame AS TimeNotBeginGame, @wTimeNotBeginAfterCreateRoom AS TimeNotBeginAfterCreateRoom, @cbCreateRight AS CreateRight,
	@wBeginFreeTime as BeginFreeTime,@wEndFreeTime as EndFreeTime

END

RETURN 0

GO
----------------------------------------------------------------------------------------------------

-- ����˽�˷�����Ϣ
CREATE PROC GSP_GS_GetPersonalRoomUserScore
	@dwUserID INT,								-- �û� I D
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN

	-- ���ط���
	SELECT * FROM PersonalRoomScoreInfo WHERE UserID = @dwUserID ORDER BY WriteTime DESC

END

RETURN 0

GO


----------------------------------------------------------------------------------------------------

-- ����˽�˷�����Ϣ
CREATE PROC GSP_GS_CloseRoomWriteDissumeTime
	@dwServerID INT,								-- �û� I D
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN
	
	--ɾ���˷������������
	DELETE FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreLocker WHERE ServerID = @dwServerID;
	-- ���ط���
	UPDATE StreamCreateTableFeeInfo SET DissumeDate = GetDATE(), RoomStatus = 2 WHERE ServerID = @dwServerID AND DissumeDate IS NULL

END

RETURN 0

GO

------------------------------------------------------
---��������
CREATE PROC GSP_MB_GetPersonalRule
	@dwKindID INT,								-- ��Ϸ I D
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ
WITH ENCRYPTION AS

BEGIN
	DECLARE @dwServerID INT						-- ���� I D
	SELECT  @dwServerID=ServerID FROM PersonalRoomInfo WHERE KindID=@dwKindID	
	IF @dwServerID IS NOT NULL
	BEGIN
		SELECT PersonalRule FROM GameRoomInfo WHERE ServerID=@dwServerID
		return 0		
	END
	ELSE 
	BEGIN		
		SET @strErrorDescribe=N'�Ҳ�������Ϸ��'
		RETURN 1
	END
END

return 0
GO

---------------------------------------------------------------