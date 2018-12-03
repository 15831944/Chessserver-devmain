
----------------------------------------------------------------------------------------------------

USE WHJHGameScoreDB
GO


----д˽�˷�������Ϣ
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GR_WritePersonalGameScore]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GR_WritePersonalGameScore]
GO



SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO


----------------------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------------------
-- ��Ϸд��
CREATE PROC GSP_GR_WritePersonalGameScore

	-- ϵͳ��Ϣ
	@dwUserID INT,								-- �û� I D
	@dwPersonalRoomID INT,					-- Լս����ID
	@dwDBQuestID INT,							-- �����ʶ
	@dwInoutIndex INT,							-- ��������

	-- ����ɼ�
	@lVariationScore BIGINT,					-- �û�����
	@lVariationGrade BIGINT,					-- �û��ɼ�
	@lVariationInsure BIGINT,					-- �û�����
	@lVariationRevenue BIGINT,					-- ��Ϸ˰��
	@lVariationWinCount INT,					-- ʤ������
	@lVariationLostCount INT,					-- ʧ������
	@lVariationDrawCount INT,					-- �;�����
	@lVariationFleeCount INT,					-- ������Ŀ
	@lVariationUserMedal INT,					-- �û�����
	@lVariationExperience INT,					-- �û�����
	@lVariationLoveLiness INT,					-- �û�����
	@dwVariationPlayTimeCount INT,				-- ��Ϸʱ��

	-- ������Ϣ
	@wKindID INT,								-- ��Ϸ I D
	@wServerID SMALLINT,						-- ���� I D
	@strClientIP NVARCHAR(15),					-- ���ӵ�ַ
	@dwRoomHostID INT,							-- ���� ID
	@dwPersonalTax INT,							-- ˽�˷�˰��
	@strPersonalRoomGUID NVARCHAR(31)			-- ˽�˷���Ψһ��ʶ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN
	
	-- �����ǰ������Ҳ�����
	DECLARE @dwExistUserID INT
	SELECT @dwExistUserID = UserID FROM WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomScoreInfo WHERE PersonalRoomGUID = @strPersonalRoomGUID AND UserID = @dwUserID  

	IF @dwExistUserID IS NULL
	BEGIN
		INSERT INTO WHJHPlatformDB..PersonalRoomScoreInfo(UserID, RoomID, Score, WinCount, LostCount, DrawCount, FleeCount, WriteTime, PersonalRoomGUID) 
												VALUES (@dwUserID, @dwPersonalRoomID, @lVariationScore,@lVariationWinCount, @lVariationLostCount, @lVariationDrawCount, @lVariationFleeCount, GETDATE(), @strPersonalRoomGUID) 
	END
	ELSE
	BEGIN
		-- �û�����
		UPDATE WHJHPlatformDBLink.WHJHPlatformDB.dbo.PersonalRoomScoreInfo SET Score=Score+@lVariationScore, WinCount=WinCount+@lVariationWinCount, LostCount=LostCount+@lVariationLostCount,
			DrawCount=DrawCount+@lVariationDrawCount, FleeCount=FleeCount+@lVariationFleeCount, WriteTime=GETDATE() 
		WHERE PersonalRoomGUID = @strPersonalRoomGUID AND UserID = @dwUserID  
	END		
END

RETURN 0

GO







