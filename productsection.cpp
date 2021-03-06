#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <sstream>

#include <QtSql>
#include <QStandardItemModel>

#include "pricedialog.h"

void MainWindow::buildProductTree()
{
    buildTree(SK_S_PROD);
}

void MainWindow::productSelected(int id)
{
    this->currentProduct = id;
    if (id == -1)
    {
        this->ui->prod_scroll->setEnabled(false);
        this->ui->prod_name->clear();
        this->ui->prod_cat->clear();
        this->ui->prod_subcat->clear();
        if (this->ui->prod_pricetable->model() != NULL)
        {
            ((QStandardItemModel *) this->ui->prod_pricetable->model())->clear();
        }
        this->ui->prod_meas->clear();
        this->ui->prod_notes->clear();

        return;
    }

    this->ui->prod_scroll->setEnabled(true);

    QSqlQuery query;
    query.prepare("SELECT name, notes, cat, subcat, meas "
                  "FROM product "
                  "WHERE id = :id" );
    query.bindValue(":id", id);
    query.exec();
    if (query.next())
    {
        this->ui->prod_name->setText(query.value("name").toString());
        this->ui->prod_notes->setPlainText(query.value("notes").toString());
        int cat = -1;
        int subcat = -1;
        int meas = -1;
        cat = (query.value("cat").isNull()) ? -1 : query.value("cat").toInt();
        subcat = (query.value("subcat").isNull()) ? -1 : query.value("subcat").toInt();
        meas = (query.value("meas").isNull()) ? -1 : query.value("meas").toInt();
        fillProductCategoryLists(cat, subcat);
        fillProductMeasurementList(meas);
        updatePriceList();
    }
}

void MainWindow::updatePriceList()
{
    QStandardItemModel * rootModel;
    if (this->ui->prod_pricetable->model() != NULL)
    {
        rootModel = (QStandardItemModel *) this->ui->prod_pricetable->model();
        rootModel->clear();
    }
    else
    {
        rootModel = new QStandardItemModel(this);
    }

    int currentPrice = -1;

    QSqlQuery query;
    query.prepare("SELECT current_price FROM product WHERE id = :prodid");
    query.bindValue(":prodid", QVariant::fromValue(this->currentProduct));
    query.exec();
    if (query.next())
    {
        currentPrice = query.value("current_price").toInt();
    }

    query.prepare("SELECT P.id, P.price / P.quantity 'cprice', D.name "
                  "FROM prod_price P LEFT JOIN price_provider D "
                  "ON P.provider = D.id "
                  "WHERE P.product = :prodid");
    query.bindValue(":prodid", QVariant::fromValue(this->currentProduct));
    query.exec();
    rootModel->setColumnCount(3);
    while (query.next())
    {
        QList<QStandardItem*> row;
        QStandardItem* price = new QStandardItem(currencyFormatter(query.value("cprice")));
        QStandardItem* notes = new QStandardItem(query.value("name").toString());
        QStandardItem* current = new QStandardItem("");

        if (query.value("id").toInt() == currentPrice)
        {
            current->setText("✓");
        }
        row << current << price << notes;

        current->setData(query.value("id"), SK_IdRole);

        rootModel->appendRow(row);

    }

    rootModel->setHorizontalHeaderLabels(QList<QString>() << tr("Current") << tr("Price") << tr("Provider"));
    this->ui->prod_pricetable->setModel(rootModel);
    updateRecipePrice();
}

void MainWindow::fillProductCategoryLists(int catId, int subCatId)
{
    fillCategoryLists(catId, subCatId, SK_S_PROD);
}

void MainWindow::prodCatSelected(int index)
{
    catSelected(index, SK_S_PROD);
}

void MainWindow::fillProductMeasurementList(int measId)
{
    this->ui->prod_meas->clear();

    QSqlQuery query;
    query.prepare("SELECT id, name FROM prod_meas");
    query.exec();
    int selected = 0, index = 1;
    this->ui->prod_meas->addItem(tr("None"), QVariant::fromValue(-1));
    while (query.next())
    {
        this->ui->prod_meas->addItem(query.value("name").toString(),
                                     QVariant::fromValue(query.value("id").toInt()));
        if (query.value("id").toInt() == measId)
            selected = index;
        index++;
    }
    this->ui->prod_meas->setCurrentIndex(selected);
}

void MainWindow::showNewPricePopup()
{
    PriceDialog* popUp = new PriceDialog(currentProduct, -1, this);
    popUp->setModal(true);
    popUp->show();
}

void MainWindow::showEditPricePopup()
{
    QModelIndexList indexes = this->ui->prod_pricetable->selectionModel()->selectedIndexes();
    if (indexes.size())
    {
        QModelIndex tableIndex = indexes.first();
        QVariant priceid = this->ui->prod_pricetable->model()->itemData(tableIndex)[SK_IdRole];
        PriceDialog* popUp = new PriceDialog(currentProduct, priceid.toInt(), this);
        popUp->setModal(true);
        popUp->show();
    }
}


void MainWindow::generalProdButtonClicked(QAbstractButton *button)
{
    switch (this->ui->prod_curprod_buttons->standardButton(button))
    {
    case QDialogButtonBox::Reset:
        resetProductData();
        break;
    case QDialogButtonBox::Save:
        saveProductData();
        break;
    default:
        break;
    }
}

void MainWindow::resetProductData()
{
    productSelected(currentProduct);
}

void MainWindow::saveProductData()
{
    QString name = this->ui->prod_name->text();
    QString notes = this->ui->prod_notes->toPlainText();
    int catId = this->ui->prod_cat->currentData().toInt();
    int subCatId = this->ui->prod_subcat->currentData().toInt();
    int measId = this->ui->prod_meas->currentData().toInt();
    QVariant catV = (catId != -1) ? QVariant::fromValue(catId) : QVariant();
    QVariant subCatV = (subCatId != -1) ? QVariant::fromValue(subCatId) : QVariant();
    QVariant measV = (measId != -1) ? QVariant::fromValue(measId) : QVariant();

    QSqlQuery query;
    query.prepare("UPDATE product SET "
                  "name = :name, "
                  "notes = :notes, "
                  "cat = :cat, "
                  "subcat = :subcat, "
                  "meas = :meas "
                  "WHERE id = :id");
    query.bindValue(":name", name);
    query.bindValue(":notes", notes);
    query.bindValue(":cat", catV);
    query.bindValue(":subcat", subCatV);
    query.bindValue(":meas", measV);
    query.bindValue(":id", currentProduct);
    query.exec();

    productSelected(currentProduct);
    buildProductTree();
    sta_loadTables();
}

void MainWindow::setCurrentPrice()
{
    QModelIndexList indexes = this->ui->prod_pricetable->selectionModel()->selectedIndexes();
    if (indexes.size())
    {
        QModelIndex tableIndex = indexes.first();
        QVariant priceid = this->ui->prod_pricetable->model()->itemData(tableIndex)[SK_IdRole];
        QSqlQuery query;
        query.prepare("UPDATE product SET current_price = :priceid WHERE id = :id");
        query.bindValue(":id", currentProduct);
        query.bindValue(":priceid", priceid);
        query.exec();

        updatePriceList();
        sta_loadTables();
    }
}

void MainWindow::deletePrice()
{
    QModelIndexList indexes = this->ui->prod_pricetable->selectionModel()->selectedIndexes();
    bool deleted = false;

    for (int i = 0; i < indexes.size(); i++)
    {
        QModelIndex tableIndex = indexes.first();
        QVariant id = this->ui->prod_pricetable->model()->itemData(tableIndex)[SK_IdRole];
        QSqlQuery query;
        query.prepare("DELETE FROM prod_price WHERE id = :id");
        query.bindValue(":id", id);
        query.exec();
        if (query.lastError().type() == QSqlError::NoError)
        {
            deleted = true;
        }
    }
    if (deleted)
    {
        updatePriceList();
        sta_loadTables();
    }
}

void MainWindow::insertNewProduct()
{
    QTreeView* tree = this->ui->prod_tree;

    QModelIndexList indexes = tree->selectionModel()->selectedIndexes();
    QVariant cat;
    QVariant subcat;

    if (!indexes.isEmpty())
    {
        QModelIndex tableIndex = indexes.at(0);
        QVariant typeSel = tree->model()->itemData(tableIndex)[SK_TypeRole];
        if (typeSel.toInt() == SK_Category)
        {
            cat = tree->model()->itemData(tableIndex)[SK_IdRole];
        }
        else if (typeSel.toInt() == SK_Subcategory)
        {
            subcat = tree->model()->itemData(tableIndex)[SK_IdRole];
            QSqlQuery query;
            query.prepare("SELECT cat FROM prod_subcat WHERE id = :subcat");
            query.bindValue(":subcat", subcat);
            query.exec();
            if (query.next() && !query.value(0).isNull())
            {
                cat = query.value(0);
            }
            else
            {
                subcat = QVariant();
            }
        }
        else if (typeSel.toInt() == SK_Product)
        {
            QVariant id = tree->model()->itemData(tableIndex)[SK_IdRole];
            QSqlQuery query;
            query.prepare("SELECT cat, subcat FROM product WHERE id = :id");
            query.bindValue(":id", id);
            query.exec();
            if (query.next() && !query.value("cat").isNull())
            {
                cat = query.value("cat");
                if (!query.value("subcat").isNull())
                {
                    subcat = query.value("subcat");
                }
            }
        }
    }

    QSqlQuery query;
    query.prepare("INSERT INTO product VALUES ( "
                  "NULL, "  // Id
                  ":newproducttext, "  // Name
                  ":cat, "  // Cat
                  ":subcat, "  // Subcat
                  "NULL, "  // Notes
                  "NULL, "  // Price
                  "NULL "  // Measurement
                  ")");
    query.bindValue(":newproducttext", tr("New Ingredient"));
    query.bindValue(":cat", cat);
    query.bindValue(":subcat", subcat);
    query.exec();

    query.prepare("SELECT last_insert_rowid()");
    query.exec();
    if (query.next())
    {
        productSelected(query.value(0).toInt());
    }
    buildProductTree();
}

void MainWindow::deleteProduct()
{
    deleteItems(SK_S_PROD);
}

