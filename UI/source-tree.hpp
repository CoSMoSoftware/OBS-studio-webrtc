#pragma once

#include <QList>
#include <QVector>
#include <QPointer>
#include <QListView>
#include <QCheckBox>
#include <QStaticText>
#include <QSvgRenderer>
#include <QAbstractListModel>
#include <obs.hpp>
#include <obs-frontend-api.h>

class QLabel;
class QCheckBox;
class QLineEdit;
class SourceTree;
class QSpacerItem;
class QHBoxLayout;
class LockedCheckBox;
class VisibilityCheckBox;
class VisibilityItemWidget;

class SourceTreeSubItemCheckBox : public QCheckBox {
	Q_OBJECT
};

class SourceTreeItem : public QWidget {
	Q_OBJECT

	friend class SourceTree;
	friend class SourceTreeModel;

	void mouseDoubleClickEvent(QMouseEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	void enterEvent(QEnterEvent *event) override;
#else
	void enterEvent(QEvent *event) override;
#endif
	void leaveEvent(QEvent *event) override;

	virtual bool eventFilter(QObject *object, QEvent *event) override;

	void Update(bool force);

	enum class Type {
		Unknown,
		Item,
		Group,
		SubItem,
	};

	void DisconnectSignals();
	void ReconnectSignals();

	Type type = Type::Unknown;

public:
	explicit SourceTreeItem(SourceTree *tree, OBSSceneItem sceneitem);
	bool IsEditing();

private:
	QSpacerItem *spacer = nullptr;
	QCheckBox *expand = nullptr;
	VisibilityCheckBox *vis = nullptr;
	LockedCheckBox *lock = nullptr;
	QHBoxLayout *boxLayout = nullptr;
	QLabel *label = nullptr;

	QLineEdit *editor = nullptr;

	std::string newName;

	SourceTree *tree;
	OBSSceneItem sceneitem;
	OBSSignal sceneRemoveSignal;
	OBSSignal itemRemoveSignal;
	OBSSignal groupReorderSignal;
	OBSSignal selectSignal;
	OBSSignal deselectSignal;
	OBSSignal visibleSignal;
	OBSSignal lockedSignal;
	OBSSignal renameSignal;
	OBSSignal removeSignal;

	virtual void paintEvent(QPaintEvent *event) override;

	void ExitEditModeInternal(bool save);

private slots:
	void Clear();

	void EnterEditMode();
	void ExitEditMode(bool save);

	void VisibilityChanged(bool visible);
	void LockedChanged(bool locked);
	void Renamed(const QString &name);

	void ExpandClicked(bool checked);

	void Select();
	void Deselect();
};

class SourceTreeModel : public QAbstractListModel {
	Q_OBJECT

	friend class SourceTree;
	friend class SourceTreeItem;

	SourceTree *st;
	QVector<OBSSceneItem> items;
	bool hasGroups = false;

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);
	void Clear();
	void SceneChanged();
	void ReorderItems();

	void Add(obs_sceneitem_t *item);
	void Remove(obs_sceneitem_t *item);
	OBSSceneItem Get(int idx);
	QString GetNewGroupName();
	void AddGroup();

	void GroupSelectedItems(QModelIndexList &indices);
	void UngroupSelectedGroups(QModelIndexList &indices);

	void ExpandGroup(obs_sceneitem_t *item);
	void CollapseGroup(obs_sceneitem_t *item);

	void UpdateGroupState(bool update);

public:
	explicit SourceTreeModel(SourceTree *st);
	~SourceTreeModel();

	virtual int rowCount(const QModelIndex &parent) const override;
	virtual QVariant data(const QModelIndex &index,
			      int role) const override;

	virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
	virtual Qt::DropActions supportedDropActions() const override;
};

class SourceTree : public QListView {
	Q_OBJECT

	bool ignoreReorder = false;

	friend class SourceTreeModel;
	friend class SourceTreeItem;

	bool textPrepared = false;
	QStaticText textNoSources;
	QSvgRenderer iconNoSources;

	OBSData undoSceneData;

	bool iconsVisible = true;

	void UpdateNoSourcesMessage();

	void ResetWidgets();
	void UpdateWidget(const QModelIndex &idx, obs_sceneitem_t *item);
	void UpdateWidgets(bool force = false);

	inline SourceTreeModel *GetStm() const
	{
		return reinterpret_cast<SourceTreeModel *>(model());
	}

public:
	inline SourceTreeItem *GetItemWidget(int idx)
	{
		QWidget *widget = indexWidget(GetStm()->createIndex(idx, 0));
		return reinterpret_cast<SourceTreeItem *>(widget);
	}

	explicit SourceTree(QWidget *parent = nullptr);

	inline bool IgnoreReorder() const { return ignoreReorder; }
	inline void Clear() { GetStm()->Clear(); }

	inline void Add(obs_sceneitem_t *item) { GetStm()->Add(item); }
	inline OBSSceneItem Get(int idx) { return GetStm()->Get(idx); }
	inline QString GetNewGroupName() { return GetStm()->GetNewGroupName(); }

	void SelectItem(obs_sceneitem_t *sceneitem, bool select);

	bool MultipleBaseSelected() const;
	bool GroupsSelected() const;
	bool GroupedItemsSelected() const;

	void UpdateIcons();
	void SetIconsVisible(bool visible);

public slots:
	inline void ReorderItems() { GetStm()->ReorderItems(); }
	inline void RefreshItems() { GetStm()->SceneChanged(); }
	void Remove(OBSSceneItem item);
	void GroupSelectedItems();
	void UngroupSelectedGroups();
	void AddGroup();
	bool Edit(int idx);
	void NewGroupEdit(int idx);

protected:
	virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
	virtual void dropEvent(QDropEvent *event) override;
	virtual void mouseMoveEvent(QMouseEvent *event) override;
	virtual void leaveEvent(QEvent *event) override;
	virtual void paintEvent(QPaintEvent *event) override;

	virtual void
	selectionChanged(const QItemSelection &selected,
			 const QItemSelection &deselected) override;
};
