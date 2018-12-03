
----------------------------------------------------------------------------------------------------

USE WHJHPlatformDB
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_QueryVideoInfo]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_QueryVideoInfo]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_QueryVideoDetails]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_QueryVideoDetails]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_QueryVideoDetailsByRoomID]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_QueryVideoDetailsByRoomID]

GO
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_MB_QueryPlayBackCodeYZ]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_MB_QueryPlayBackCodeYZ]
GO

SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

----------------------------------------------------------------------------------------------------

-- ��ѯ��Ƶ�б�
CREATE PROC GSP_MB_QueryVideoInfo
	@iQueryType INT,								-- 0:Լս 1:��ͨ				
	@dwUserID	INT,								-- �û� I D
	@dwPlayBack INT,								-- �ط����0
	@dwPersonalRoomID	INT,						-- Լս����ID
	@wIndexBegin INT,								-- ������ʼ
	@wIndexEnd INT,									-- ��������
	@strErrorDescribe NVARCHAR(127) OUTPUT			-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN		
	IF @dwPlayBack=0
	BEGIN
		-- ��ѯ�û�
		IF not exists(SELECT * FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsInfo WHERE UserID=@dwUserID)
		BEGIN
			SET @strErrorDescribe = N'��Ǹ������û���Ϣ�����ڣ�'
			return 1
		END
		IF @iQueryType=0
		BEGIN
			IF @dwPersonalRoomID <> 0
			BEGIN
				SELECT a.UserID as UserID,c.NickName as NickName,c.GameID as GameID,c.FaceID as FaceID,c.CustomID as CustomID,a.PersonalRoomGUID as PersonalGUID,a.RoomID as RoomID,a.Score as TotalScore,a.WriteTime as CreateTime,
				a.ChairID as ChairID,a.KindID as KindID,c.Gender as Gender,0 as QueryType,
				(case when (EXISTS(select * from [WHJHPlatformDB].[dbo].[StreamCreateTableFeeInfo] where UserID=a.UserID and RoomID=a.RoomID )) then 1 else 0 end) AS CreateRoom 
				FROM [WHJHPlatformDB].[dbo].[PersonalRoomScoreInfo]  a ,
				(select UserID,PersonalRoomGUID from [WHJHPlatformDB].[dbo].[PersonalRoomScoreInfo] where RoomID=@dwPersonalRoomID) b ,
				[WHJHAccountsDB].[dbo].[AccountsInfo] c			
				where a.PersonalRoomGUID = b.PersonalRoomGUID and c.UserID = a.UserID and DateDiff(d,a.WriteTime,GetDate()) < 1 order by a.WriteTime desc
			END
			else
			BEGIN
				SELECT a.UserID as UserID,c.NickName as NickName,c.GameID as GameID,c.FaceID as FaceID,c.CustomID as CustomID,a.PersonalRoomGUID as PersonalGUID,a.RoomID as RoomID,a.Score as TotalScore,a.WriteTime as CreateTime,
				a.ChairID as ChairID,a.KindID as KindID,c.Gender as Gender,0 as QueryType,
				(case when (EXISTS(select * from [WHJHPlatformDB].[dbo].[StreamCreateTableFeeInfo] where UserID=a.UserID and RoomID=a.RoomID )) then 1 else 0 end) AS CreateRoom 
				FROM [WHJHPlatformDB].[dbo].[PersonalRoomScoreInfo]  a ,
				(select top (@wIndexEnd) PersonalRoomGUID from [WHJHPlatformDB].[dbo].[RecordBackInfo] where UserID=@dwUserID and GamesNum = 1 and DateDiff(d,Dates,GetDate()) <= 1 and (PersonalRoomGUID NOT IN (select top (@wIndexBegin) PersonalRoomGUID from [WHJHPlatformDB].[dbo].[RecordBackInfo] where UserID=@dwUserID and GamesNum = 1 and DateDiff(d,Dates,GetDate()) <= 1 order by Dates desc )) order by Dates desc) b ,--group by PersonalRoomGUID having count(PersonalRoomGUID) > 0) b , 			
				[WHJHAccountsDB].[dbo].[AccountsInfo] c			
				where a.PersonalRoomGUID = b.PersonalRoomGUID and c.UserID = a.UserID and DateDiff(d,a.WriteTime,GetDate()) < 1 order by a.WriteTime desc
			END
		END
		ELSE
		BEGIN
			SELECT a.UserID as UserID,c.NickName as NickName,c.GameID as GameID,c.FaceID as FaceID,c.CustomID as CustomID,a.VideoNumber as VideoNumber,a.Score as TotalScore,a.BuildVideoTime as CreateTime,			 
			a.ChairID as ChairID,a.KindID as KindID,c.Gender as Gender,1 as QueryType
			FROM [WHJHTreasureDB].[dbo].[RecordVideoPlayerInfo]  a ,					
			(select top (@wIndexEnd) VideoNumber from [WHJHTreasureDB].[dbo].[RecordVideoPlayerInfo] where UserID=@dwUserID and (VideoNumber NOT IN (select top (@wIndexBegin) VideoNumber from [WHJHTreasureDB].[dbo].[RecordVideoPlayerInfo]  where UserID=@dwUserID))) b ,		
			[WHJHAccountsDB].[dbo].[AccountsInfo] c			
			where c.UserID = a.UserID and b.VideoNumber=a.VideoNumber and DateDiff(d,a.BuildVideoTime,GetDate()) < 1			
		END	
	END	
	ELSE
	BEGIN	
		IF EXISTS (select *  from [WHJHPlatformDB].[dbo].[PersonalRoomScoreInfo] where PlayBackCode=@dwPlayBack)	
		BEGIN
			SELECT a.UserID as UserID,c.NickName as NickName,c.GameID as GameID,c.FaceID as FaceID,c.CustomID as CustomID,a.PersonalRoomGUID as PersonalGUID,a.RoomID as RoomID,a.Score as TotalScore,a.WriteTime as CreateTime,
			a.ChairID as ChairID,a.KindID as KindID,c.Gender as Gender,b.UserID as PlayUserID,0 as QueryType,
			(case when (EXISTS(select * from [WHJHPlatformDB].[dbo].[StreamCreateTableFeeInfo] where UserID=a.UserID and RoomID=a.RoomID )) then 1 else 0 end) AS CreateRoom 
			FROM [WHJHPlatformDB].[dbo].[PersonalRoomScoreInfo]  a ,
			(select UserID,PersonalRoomGUID from [WHJHPlatformDB].[dbo].[PersonalRoomScoreInfo] where PlayBackCode=@dwPlayBack) b ,
			[WHJHAccountsDB].[dbo].[AccountsInfo] c			
			where a.PersonalRoomGUID = b.PersonalRoomGUID and c.UserID = a.UserID and DateDiff(d,a.WriteTime,GetDate()) < 1
		END
		ELSE		
		BEGIN
			SELECT a.UserID as UserID,c.NickName as NickName,c.GameID as GameID,c.FaceID as FaceID,c.CustomID as CustomID,a.VideoNumber as VideoNumber,a.Score as TotalScore,a.BuildVideoTime as CreateTime,			 
			a.ChairID as ChairID,a.KindID as KindID,c.Gender as Gender,b.UserID as PlayUserID,1 as QueryType
			FROM [WHJHTreasureDB].[dbo].[RecordVideoPlayerInfo]  a ,	
			(select UserID,VideoNumber from [WHJHTreasureDB].[dbo].[RecordVideoPlayerInfo]  where PlayBackCode=@dwPlayBack) b ,		
			[WHJHAccountsDB].[dbo].[AccountsInfo] c			
			where c.UserID = a.UserID and b.VideoNumber=a.VideoNumber and DateDiff(d,a.BuildVideoTime,GetDate()) < 1
		END			
	END
END

RETURN 0

GO
----------------------------------------------------------------------------------------------------

-- ��ѯ��Ƶ����
CREATE PROC GSP_MB_QueryVideoDetails
	@szPersonalGUID	NVARCHAR(31),					-- Լս��Ψһ I D
	@strErrorDescribe NVARCHAR(127) OUTPUT			-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN	
	SELECT a.ID as VideoNumber,a.UserID as UserID,b.NickName as NickName,a.Score as Score,
		a.GamesNum as GameNum,@szPersonalGUID as PersonalGUID,a.GameMode as GameMode, a.LoopCount as LoopCount
	FROM [WHJHPlatformDB].[dbo].[RecordBackInfo] a,[WHJHAccountsDB].[dbo].[AccountsInfo] b
	where PersonalRoomGUID=@szPersonalGUID and a.UserID = b.UserID
END 

RETURN 0

GO
----------------------------------------------------------------------------------------------------
-- ��ѯ��Ƶ����
CREATE PROC GSP_MB_QueryVideoDetailsByRoomID
	@dwPersonalRoomID	INT,							-- Լս��I D
	@strErrorDescribe	NVARCHAR(127) OUTPUT			-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN	
	SELECT a.ID as VideoNumber,a.UserID as UserID,b.NickName as NickName,a.Score as Score,
		a.GamesNum as GameNum,a.PersonalRoomGUID as PersonalGUID,a.GameMode as GameMode, a.LoopCount as LoopCount
	FROM [WHJHPlatformDB].[dbo].[RecordBackInfo] a,[WHJHAccountsDB].[dbo].[AccountsInfo] b
	where RoomID=@dwPersonalRoomID and a.UserID = b.UserID
END 

RETURN 0

GO
----------------------------------------------------------------------------------------------------
-- ��ѯ�ط���
CREATE PROC GSP_MB_QueryPlayBackCodeYZ
	@dwUserID		INT,							-- �û� I D
	@szPersonalGUID	NVARCHAR(31),					-- Լս��Ψһ I D
	@strErrorDescribe NVARCHAR(127) OUTPUT			-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN
	declare @bFind	INT
	declare @Code	INT
	SET @Code=0      
	select @Code=PlayBackCode from  [WHJHPlatformDB].[dbo].[PersonalRoomScoreInfo] where PersonalRoomGUID=@szPersonalGUID and UserID=@dwUserID and DateDiff(d,WriteTime,GetDate()) <= 10
	
	IF @Code=0
	BEGIN
		SET @bFind=0       
		WHILE @bFind=0    
		BEGIN
			SET @Code = cast(ceiling(rand() * 899999) as int)+100000
			IF NOT EXISTS (select * from  [WHJHPlatformDB].[dbo].[PersonalRoomScoreInfo] where PlayBackCode=@Code and DateDiff(d,WriteTime,GetDate()) <= 10)
			BEGIN
				SET @bFind = 1  
				UPDATE [WHJHPlatformDB].[dbo].[PersonalRoomScoreInfo] SET PlayBackCode =@Code WHERE UserID=@dwUserID and PersonalRoomGUID=@szPersonalGUID   
			END
		END 
	END	
	
	SELECT @Code as Code
END

RETURN 0

GO
----------------------------------------------------------------------------------------------------