/**
 * Copyright 2011 Kurtis L. Nusbaum
 * 
 * This file is part of UDJ.
 * 
 * UDJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * UDJ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with UDJ.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "LibraryView.hpp"
#include "Utils.hpp"
#include "MusicModel.hpp"
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QMenu>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSortFilterProxyModel>

namespace UDJ{


LibraryView::LibraryView(DataStore *dataStore, QWidget* parent):
  QTableView(parent),
  dataStore(dataStore)
{
  libraryModel = new MusicModel(getDataQuery(), dataStore, this);
  proxyModel = new QSortFilterProxyModel(this);
  proxyModel->setSourceModel(libraryModel);
  proxyModel->setFilterKeyColumn(-1);
  proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);


  verticalHeader()->hide();
  horizontalHeader()->setStretchLastSection(true);
  setModel(proxyModel);
  setSortingEnabled(true);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setContextMenuPolicy(Qt::CustomContextMenu);
  configureColumns();
  createActions();
  connect(dataStore, SIGNAL(libSongsModified()), libraryModel, SLOT(refresh()));
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
    this, SLOT(handleContextMenuRequest(const QPoint&)));
}

void LibraryView::configureColumns(){
  QSqlRecord record = libraryModel->record();
  int idIndex = record.indexOf(DataStore::getLibIdColName());
  int isDeletedIndex = record.indexOf(DataStore::getLibIsDeletedColName());
  int syncStatusIndex = record.indexOf(DataStore::getLibSyncStatusColName());
  int durationIndex = record.indexOf(DataStore::getLibDurationColName());
  setColumnHidden(idIndex, true);
  setColumnHidden(isDeletedIndex, true); 
  setColumnHidden(syncStatusIndex, true); 
  resizeColumnToContents(durationIndex);
}


void LibraryView::createActions(){
  deleteSongAction = new QAction(getDeleteContextMenuItemName(), this);
  connect(
    deleteSongAction, 
    SIGNAL(triggered()), 
    this, 
    SLOT(deleteSongs()));
}


void LibraryView::handleContextMenuRequest(const QPoint &pos){
  QMenu contextMenu(this);

  QSqlQuery songLists = dataStore->getSongLists();
  if(songLists.next()){
    QMenu *songListsMenu = new QMenu(tr("Add To Song Lists"), this);
    contextMenu.addMenu(songListsMenu);
    QSqlRecord currentRecord;
    do{
      currentRecord = songLists.record();
      QAction *addedAction = songListsMenu->addAction(
        currentRecord.value(DataStore::getSongListNameColName()).toString());
      addedAction->setData(
        currentRecord.value(DataStore::getSongListIdColName()));
    }while(songLists.next());
  }
  contextMenu.addAction(deleteSongAction);
  QAction *selected = contextMenu.exec(QCursor::pos());
  if(selected != NULL){
    QVariant data = selected->data();
    if(data.isValid()){
      addSongsToSongList(data.value<song_list_id_t>()); 
    }
  }
}


void LibraryView::deleteSongs(){
  dataStore->removeSongsFromLibrary(
    Utils::getSelectedIds<library_song_id_t>(
      this,
      libraryModel,
      DataStore::getLibIdColName(),
      proxyModel));
}

void LibraryView::addSongsToSongList(song_list_id_t songListId){
  dataStore->addSongsToSongList(
    songListId,
    Utils::getSelectedIds<library_song_id_t>(
      this,
      libraryModel,
      DataStore::getLibIdColName(),
      proxyModel));
}

void LibraryView::filterContents(const QString& filter){
  proxyModel->setFilterFixedString(filter);
}


}//end namespace
