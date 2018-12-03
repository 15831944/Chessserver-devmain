
----------------------------------------------------------------------------------------------------

USE WHJHPlatformDB
GO
IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_LoadGamePropertyTypeItem]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_LoadGamePropertyTypeItem]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_LoadGamePropertyRelatItem]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_LoadGamePropertyRelatItem]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_LoadGamePropertyItem]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_LoadGamePropertyItem]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_LoadGamePropertySubItem]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_LoadGamePropertySubItem]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_BuyProperty]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_BuyProperty]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_LoadUserGameBuff]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_LoadUserGameBuff]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_LoadUserGameTrumpet]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_LoadUserGameTrumpet]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_QuerySendPresent]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_QuerySendPresent]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_UserSendPresentByID]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_UserSendPresentByID]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_UserSendPresentByNickName]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_UserSendPresentByNickName]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_GetSendPresent]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_GetSendPresent]
GO

IF EXISTS (SELECT * FROM DBO.SYSOBJECTS WHERE ID = OBJECT_ID(N'[dbo].[GSP_GP_PropertQuerySingle]') and OBJECTPROPERTY(ID, N'IsProcedure') = 1)
DROP PROCEDURE [dbo].[GSP_GP_PropertQuerySingle]
GO

SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

----------------------------------------------------------------------------------------------------

-- ��������
CREATE PROC GSP_GP_LoadGamePropertyTypeItem
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN

	SELECT * FROM GamePropertyType(NOLOCK) WHERE Nullity=0 AND TagID = 0
	
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------

-- ���ع�ϵ
CREATE PROC GSP_GP_LoadGamePropertyRelatItem
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN

	SELECT * FROM GamePropertyRelat(NOLOCK) WHERE  TagID = 0
	
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------

-- ���ص���
CREATE PROC GSP_GP_LoadGamePropertyItem
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN
	-- ���ص���
	SELECT * FROM GameProperty(NOLOCK) WHERE Nullity=0 AND Kind <> 11
	
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------

-- ���ص���
CREATE PROC GSP_GP_LoadGamePropertySubItem
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN
	-- ���ص���
	SELECT * FROM GamePropertySub(NOLOCK) 
	
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------

SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO


-- ���ѵ���
CREATE PROC GSP_GP_BuyProperty
	@dwUserID INT,								-- �û���ʶ
	@dwPropertyID INT,							-- ���߱�ʶ
	@iDiamondCount INT,							-- ��ʯ����
	@strPassword NCHAR(32),						-- �û�����
	@strClientIP NVARCHAR(15),					-- ���ӵ�ַ
	@strMachineID NVARCHAR(32),					-- ������ʶ
	@strErrorDescribe NVARCHAR(127) OUTPUT		-- �����Ϣ

WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN

	-- ��������
	DECLARE @ExchangeRatio INT
	DECLARE @iProPerty INT	
	DECLARE @Discount AS SMALLINT
	DECLARE @PropertyName AS NVARCHAR(31)
	DECLARE @Nullity BIT
	DECLARE @CurrMemberOrder SMALLINT	
	
	SELECT @Nullity=Nullity FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsInfo WHERE UserID=@dwUserID AND LogonPass=@strPassword

	-- �û��ж�
	IF @Nullity IS NULL
	BEGIN
		SET @strErrorDescribe=N'�ʺŲ����ڣ�����ϵ�ͻ����������˽���ϸ�����'
		RETURN 1
	END	

	-- �ʺŽ�ֹ
	IF @Nullity<>0
	BEGIN
		SET @strErrorDescribe=N'�ʺ���ʱ���ڶ���״̬������ϵ�ͻ����������˽���ϸ�����'
		RETURN 2
	END

	-- ��ʼ��Ϣ
	SET	@iProPerty= 0
	SET @ExchangeRatio = 0

	-- �����ж�
	SELECT @PropertyName=Name, @ExchangeRatio=ExchangeGoldRatio	FROM GameProperty(NOLOCK) WHERE Nullity=0 AND ID=@dwPropertyID
	IF @PropertyName IS NULL
	BEGIN
		SET @strErrorDescribe=N'����ĵ�����Ʒ�����ڻ�������ά���У�'
		RETURN 3
	END
		
	IF @ExchangeRatio = 0
	BEGIN
		SET @strErrorDescribe=N'�����߲�֧�ִ��ֹ���ʽ��'
		RETURN 3
	END

	-- ���߼���
	SET @iProPerty=@iDiamondCount*@ExchangeRatio
	
	-- ��ȡ����
	DECLARE @Gold BIGINT
	SELECT @Gold=InsureScore FROM WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreInfo WHERE UserID=@dwUserID
	
	-- �����ж�
	IF @Gold<@iProPerty OR @Gold IS NULL
	BEGIN
		-- ������Ϣ
		SET @strErrorDescribe=N'���н�����㣬�������д����㹻�Ľ�Һ��ٴγ��ԣ�'
		RETURN 4
	END
	
	-- ���п۷�
	UPDATE WHJHTreasureDBLink.WHJHTreasureDB.dbo.GameScoreInfo SET InsureScore=InsureScore-@iProPerty WHERE UserID=@dwUserID
	
	--���뱳��
	UPDATE WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsPackage SET GoodShowID=0, GoodsCount=GoodsCount+ @iDiamondCount, PushTime=GETDATE() WHERE UserID=@dwUserID AND GoodsID=@dwPropertyID
	IF @@ROWCOUNT = 0
	BEGIN
		INSERT WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsPackage(UserID,GoodsID,GoodShowID,GoodsCount,PushTime)	VALUES(@dwUserID, @dwPropertyID,0,@iDiamondCount,GETDATE())
	END
	--END

	--�����¼
	INSERT WHJHRecordDBLink.WHJHRecordDB.dbo.RecordBuyNewProperty(UserID,PropertyID,PropertyName,PropertyPrice,BuyNum,BeforeDiamond,Diamond,ClinetIP,CollectDate)
	VALUES(@dwUserID,@dwPropertyID,@PropertyName,@ExchangeRatio,@iDiamondCount,@iProPerty,@Gold,@strClientIP,GETDATE())
		
	IF @Gold IS NULL SET @Gold=0
	SET @strErrorDescribe=N'��ϲ��������' + @PropertyName + N' ��' + CAST(@iDiamondCount AS NVARCHAR) +N' ' + N'�ɹ���'
	
	--�����¼
	SELECT @dwPropertyID AS PropertyID,@iDiamondCount AS ItemCount,@Gold-@iProPerty  as Diamond
	
	RETURN 0
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------

SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

-- ����Buff
CREATE PROC GSP_GP_LoadUserGameBuff
	@dwUserID	INT						-- �û� I D
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN
	--ɾ������
	DELETE WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsInfoPoperty WHERE UserID=@dwUserID and dateadd(second,UseResultsValidTime,UseTime) < GETDATE()
	--��Ч��ѯ
	SELECT * FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsInfoPoperty  WHERE UserID=@dwUserID and dateadd(second,UseResultsValidTime,UseTime) > GETDATE()
	RETURN 0
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------

SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

-- ����Buff
CREATE PROC GSP_GP_LoadUserGameTrumpet
	@dwUserID	INT						-- �û� I D
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN
	DECLARE @TrumpetCount INT 
	DECLARE @TyphonCount INT 
	SET @TrumpetCount = 0
	SET @TyphonCount = 0
	SELECT @TrumpetCount=GoodsCount FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsPackage WHERE UserID=@dwUserID and GoodsID=307
	SELECT @TyphonCount=GoodsCount FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsPackage WHERE UserID=@dwUserID and GoodsID=306
	SELECT @TrumpetCount as TrumpetCount, @TyphonCount as TyphonCount
END

RETURN 0

GO
----------------------------------------------------------------------------------------------------


SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

--���͵���
CREATE PROC GSP_GP_QuerySendPresent
	@dwUserID				INT			-- �û�ID
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN

	SELECT * FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsSendPresent AS a, GameProperty AS b 
	WHERE a.ReceiverUserID=@dwUserID and a.PropID = b.ID
	
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------

SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

--���͵��� ������ϷID
CREATE PROC GSP_GP_UserSendPresentByID
	@dwUserID				INT,			-- �������û�ID
	@dwReceiverGameID		INT,			-- ��������ϷID
	@dwPropID				INT,			-- ���͵ĵ���ID
	@dwPropCount			INT,			-- ���͵�����
	@strClientIP			NVARCHAR(15),	-- �����ߵ�IP 
	@strErrorDescribe NVARCHAR(127) OUTPUT	-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN

	--��ѯ�û�
	DECLARE @dwReceiverUserID INT
	SELECT @dwReceiverUserID=UserID FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsInfo WHERE GameID=@dwReceiverGameID
	IF @dwReceiverUserID is null
	BEGIN
		set @strErrorDescribe = N'���Ҳ�����������Ϣ'
		RETURN 1
	END

	if @dwPropCount <= 0
	BEGIN
		set @strErrorDescribe = N'����ĵ���������Ϣ����'
		RETURN 2
	END
	
	IF @dwReceiverUserID=@dwUserID
	BEGIN
		set @strErrorDescribe = N'�����Լ����͸��Լ�'
		RETURN 3
	END
	
	--���ҵ���
	DECLARE @UserPropCount INT
	SELECT @UserPropCount=GoodsCount FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsPackage where UserID=@dwUserID and GoodsID=@dwPropID
	IF @UserPropCount is null or @UserPropCount < @dwPropCount
	BEGIN
		set @strErrorDescribe = N'�����ߵı���û���㹻�ĸõ��ߣ���������'
		RETURN 4
	END
	
	--��������
	UPDATE WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsPackage set GoodsCount=GoodsCount-@dwPropCount where UserID=@dwUserID and GoodsID=@dwPropID
	
	--����״̬
	INSERT WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsSendPresent(UserID, ReceiverUserID, PropID, PropCount, SendTime, PropStatus, ClientIP) 
	VALUES (@dwUserID, @dwReceiverUserID, @dwPropID, @dwPropCount, GETDATE(), 0, @strClientIP)
	
	--���ͼ�¼
	INSERT WHJHRecordDBLink.WHJHRecordDB.dbo.RecordUserSendPresent(UserID,ReceiverUserID,PropID,PropCount,SendTime,PropStatus,ClientIP)
	VALUES(@dwUserID,@dwReceiverUserID,@dwPropID,@dwPropCount,GETDATE(),0,@strClientIP)
	
	set @strErrorDescribe = N'���ͳɹ�'
	
	SELECT @dwReceiverUserID AS RecvUserID
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------

SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

--���͵��� ���� �ǳ�
CREATE PROC GSP_GP_UserSendPresentByNickName
	@dwUserID				INT,			-- �������û�ID
	@strReceiverNickName	NVARCHAR(16),	-- �������ǳ�
	@dwPropID				INT,			-- ���͵ĵ���ID
	@dwPropCount			INT,			-- ���͵�����
	@strClientIP			NVARCHAR(16),	-- �����ߵ�IP 
	@strErrorDescribe NVARCHAR(64) OUTPUT	-- �����Ϣ
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN

	--��ѯ�û�
	DECLARE @dwReceiverUserID INT
	SELECT @dwReceiverUserID=UserID FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsInfo WHERE NickName=@strReceiverNickName
	IF @dwReceiverUserID is null
	BEGIN
		set @strErrorDescribe = N'���Ҳ�����������Ϣ'
		RETURN 1
	END
	
	if @dwPropCount <= 0
	BEGIN
		set @strErrorDescribe = N'����ĵ���������Ϣ����'
		RETURN 2
	END
	
	IF @dwReceiverUserID=@dwUserID
	BEGIN
		set @strErrorDescribe = N'�����Լ����͸��Լ�'
		RETURN 3
	END
	
	--���������߱�����û�и������ĵ���
	DECLARE @UserPropCount INT
	SELECT @UserPropCount=GoodsCount FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsPackage where UserID=@dwUserID and GoodsID=@dwPropID
	IF @UserPropCount is null or @UserPropCount < @dwPropCount
	BEGIN
		set @strErrorDescribe = N'�����ߵı���û���㹻�ĸõ��ߣ���������'
		RETURN 4
	END
	
	--���������߱����ĵ���
	UPDATE WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsPackage set GoodsCount=GoodsCount-@dwPropCount where UserID=@dwUserID and GoodsID=@dwPropID
	
	--д�û�����״̬
	INSERT WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsSendPresent(UserID, ReceiverUserID, PropID, PropCount, SendTime, PropStatus, ClientIP) 
	VALUES (@dwUserID, @dwReceiverUserID, @dwPropID, @dwPropCount, GETDATE(), 0, @strClientIP)
	
	--���ͼ�¼
	INSERT WHJHRecordDBLink.WHJHRecordDB.dbo.RecordUserSendPresent(UserID,ReceiverUserID,PropID,PropCount,SendTime,PropStatus,ClientIP)
	VALUES(@dwUserID,@dwReceiverUserID,@dwPropID,@dwPropCount,GETDATE(),0,@strClientIP)
	
	set @strErrorDescribe = N'���ͳɹ�'
	
	SELECT @dwReceiverUserID AS RecvUserID
	
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------

--��ȡ����
SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

--���͵���
CREATE PROC GSP_GP_GetSendPresent
	@dwUserID				INT,			-- �û�ID
	@szPassword			NVARCHAR(33),		-- �û�����
	@strClientIP		NVARCHAR(15)		-- �ͻ�IP 
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN

	DECLARE @Password NVARCHAR(33)
	SELECT @Password = LogonPass FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsInfo where UserID=@dwUserID
	--��ѯ����
	IF @Password is null
		RETURN 1
	
	--�������
	IF @szPassword != @Password
		RETURN 2
	
	DECLARE @PresentCount INT
	SELECT @PresentCount = COUNT(*) FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsSendPresent
	WHERE ReceiverUserID=@dwUserID and PropStatus=0
	--û����Ʒ
	IF @PresentCount = 0
		RETURN 3
	
	--���뱳��
	DECLARE @PropID INT
	DECLARE @PropCount INT
	DECLARE @ItemCount INT
	DECLARE auth_cur CURSOR FOR
	SELECT PropID,PropCount FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsSendPresent AS a, GameProperty AS b 
	WHERE a.ReceiverUserID=@dwUserID and a.PropID = b.ID and a.PropStatus=0
	OPEN auth_cur
	FETCH NEXT FROM auth_cur INTO @PropID,@PropCount
	WHILE (@@fetch_status=0)
	BEGIN
		SELECT @ItemCount=COUNT(*) FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsPackage WHERE UserID=@dwUserID and GoodsID=@PropID
		IF @ItemCount = 0
		BEGIN
			INSERT WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsPackage(UserID,GoodsID,GoodShowID,GoodsSortID,GoodsCount,PushTime)
			VALUES(@dwUserID,@PropID,0,0,@PropCount,GETDATE())
		END
		ELSE
		BEGIN
			UPDATE WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsPackage SET GoodsCount=GoodsCount+@PropCount
			WHERE UserID=@dwUserID and GoodsID=@PropID
		END
		FETCH NEXT FROM auth_cur INTO @PropID,@PropCount
	END
	close auth_cur
	deallocate auth_cur
	
	--����״̬
	UPDATE WHJHRecordDBLink.WHJHRecordDB.dbo.RecordUserSendPresent SET ReceiveTime=GETDATE(), ReceiverClientIP=@strClientIP, PropStatus=1 
	WHERE  ReceiverUserID = @dwUserID and PropStatus=0
	
	--��ѯ����
	SELECT * FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsSendPresent AS a, GameProperty AS b 
	WHERE a.ReceiverUserID=@dwUserID and a.PropID = b.ID and a.PropStatus=0
	
	--ɾ��״̬
	DELETE WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsSendPresent WHERE  ReceiverUserID=@dwUserID and PropStatus=0
	
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------


SET QUOTED_IDENTIFIER ON 
GO

SET ANSI_NULLS ON 
GO

--���͵���
CREATE PROC GSP_GP_PropertQuerySingle
	@dwUserID INT,								-- �û���ʶ
	@dwPropertyID INT,							-- ���߱�ʶ
	@strPassword NCHAR(32)						-- �û�����
WITH ENCRYPTION AS

-- ��������
SET NOCOUNT ON

-- ִ���߼�
BEGIN

	DECLARE @Nullity INT
	SELECT @Nullity=Nullity FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsInfo WHERE UserID=@dwUserID AND DynamicPass=@strPassword

	-- �û��ж�
	IF @Nullity IS NULL
	BEGIN
		RETURN 1
	END	

	-- �ʺŽ�ֹ
	IF @Nullity<>0
	BEGIN
		RETURN 2
	END
	DECLARE @GoodsCount INT
	SET @GoodsCount = 0
	SELECT @GoodsCount = GoodsCount	FROM WHJHAccountsDBLink.WHJHAccountsDB.dbo.AccountsPackage WHERE UserID=@dwUserID and GoodsID=@dwPropertyID
	
	SELECT @dwPropertyID AS PropertyID,@GoodsCount AS ItemCount
	
END

RETURN 0

GO

----------------------------------------------------------------------------------------------------