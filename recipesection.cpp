#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <sstream>

#include <QtSql>
#include <QStandardItemModel>

#include "recipedialog.h"

void MainWindow::buildRecipeTree()
{
    buildTree(SK_S_REC);
}

void MainWindow::recipeSelected(int id)
{
    this->currentRecipe = id;
    if (id == -1)
    {
        this->ui->rec_scroll->setEnabled(false);
        this->ui->rec_name->clear();
        this->ui->rec_cat->clear();
        this->ui->rec_subcat->clear();
        if (this->ui->rec_ing->model() != NULL)
        {
            ((QStandardItemModel *) this->ui->rec_ing->model())->clear();
        }
        this->ui->rec_elab->clear();
        this->ui->rec_servs->clear();
        this->ui->rec_price->setText("--");
        return;
    }

    this->ui->rec_scroll->setEnabled(true);

    QSqlQuery query;
    query.prepare("SELECT name, cat, subcat, preparation, servings "
                  "FROM recipe "
                  "WHERE id = :id" );
    query.bindValue(":id", id);
    query.exec();
    if (query.next())
    {
        this->ui->rec_name->setText(query.value("name").toString());
        this->ui->rec_elab->setPlainText(query.value("preparation").toString());
        int cat = -1;
        int subcat = -1;
        int servs = -1;
        cat = (query.value("cat").isNull()) ? -1 : query.value("cat").toInt();
        subcat = (query.value("subcat").isNull()) ? -1 : query.value("subcat").toInt();
        servs = (query.value("servings").isNull()) ? 0 : query.value("servings").toInt();
        this->ui->rec_servs->setValue(servs);
        fillRecipeCategoryLists(cat, subcat);
        updateIngredientsList();
        updateRecipePrice();
    }
}

void MainWindow::updateRecipePrice()
{
    if (this->currentRecipe < 0)
    {
        return;
    }
    QSqlQuery query;
    query.prepare("SELECT price "
                  "FROM C_recipe_price "
                  "WHERE id = :id" );
    query.bindValue(":id", this->currentRecipe);
    query.exec();
    if (query.next())
    {
        if (!query.value("price").isNull())
        {
            this->ui->rec_price->setText(currencyFormatter(query.value("price")));
            return;
        }
    }
    this->ui->rec_price->setText("--");
}

void MainWindow::fillRecipeCategoryLists(int catId, int subCatId)
{
    fillCategoryLists(catId, subCatId, SK_S_REC);
}

void MainWindow::recCatSelected(int index)
{
    catSelected(index, SK_S_REC);
}

void MainWindow::generalRecButtonClicked(QAbstractButton *button)
{
    switch (this->ui->rec_currec_buttons->standardButton(button))
    {
    case QDialogButtonBox::Reset:
        resetRecipeData();
        break;
    case QDialogButtonBox::Save:
        saveRecipeData();
        break;
    default:
        break;
    }
}


void MainWindow::resetRecipeData()
{
    recipeSelected(currentRecipe);
}

void MainWindow::saveRecipeData()
{
    QString name = this->ui->rec_name->text();
    QString elab = this->ui->rec_elab->toPlainText();
    int catId = this->ui->rec_cat->currentData().toInt();
    int subCatId = this->ui->rec_subcat->currentData().toInt();
    QVariant catV = (catId != -1) ? QVariant::fromValue(catId) : QVariant();
    QVariant subCatV = (subCatId != -1) ? QVariant::fromValue(subCatId) : QVariant();
    QVariant servs = this->ui->rec_servs->value();

    QSqlQuery query;
    query.prepare("UPDATE recipe SET "
                  "name = :name, "
                  "preparation = :elab, "
                  "cat = :cat, "
                  "subcat = :subcat, "
                  "servings = :servs "
                  "WHERE id = :id");
    query.bindValue(":name", name);
    query.bindValue(":elab", elab);
    query.bindValue(":cat", catV);
    query.bindValue(":subcat", subCatV);
    query.bindValue(":servs", servs);
    query.bindValue(":id", currentRecipe);
    query.exec();

    recipeSelected(currentRecipe);
    buildRecipeTree();
    sta_loadTables();
}

void MainWindow::insertNewRecipe()
{
    QTreeView* tree = this->ui->rec_tree;

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
            query.prepare("SELECT cat FROM recipe_subcat WHERE id = :subcat");
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
        else if (typeSel.toInt() == SK_Recipe)
        {
            QVariant id = tree->model()->itemData(tableIndex)[SK_IdRole];
            QSqlQuery query;
            query.prepare("SELECT cat, subcat FROM recipe WHERE id = :id");
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
    query.prepare("INSERT INTO recipe VALUES ( "
                  "NULL, "  // Id
                  ":newrecipetext, "  // Name
                  ":cat, "  // Cat
                  ":subcat, "  // Subcat
                  "NULL, "  // preparation
                  "NULL "   // servings
                  ")");
    query.bindValue(":newrecipetext", tr("New Recipe"));
    query.bindValue(":cat", cat);
    query.bindValue(":subcat", subcat);
    query.exec();

    query.prepare("SELECT last_insert_rowid()");
    query.exec();
    if (query.next())
    {
        recipeSelected(query.value(0).toInt());
    }
    buildRecipeTree();
    sta_loadTables();
}


void MainWindow::deleteRecipe()
{
    deleteItems(SK_S_REC);
    sta_loadTables();
}

void MainWindow::updateIngredientsList()
{
    QStandardItemModel * rootModel;
    if (this->ui->rec_ing->model() != NULL)
    {
        rootModel = (QStandardItemModel *) this->ui->rec_ing->model();
        rootModel->clear();
    }
    else
    {
        rootModel = new QStandardItemModel(this);
    }

    QSqlQuery query;

    query.prepare("SELECT RP.product, M.name 'meas', P.name, RP.quantity "
                  "FROM recipe_product RP "
                  "JOIN product P "
                  "LEFT JOIN prod_meas M "
                  "ON P.meas = M.id "
                  "WHERE RP.recipe = :recid "
                  "AND P.id = RP.product");
    query.bindValue(":recid", QVariant::fromValue(this->currentRecipe));
    query.exec();
    rootModel->setColumnCount(2);
    while (query.next())
    {
        QList<QStandardItem*> row;
        QStandardItem* quantity = new QStandardItem(query.value("quantity").toString() + " " +
                                                    query.value("meas").toString());
        QStandardItem* product = new QStandardItem(query.value("name").toString());
        row << quantity << product;
        quantity->setData(query.value("product"), SK_IdRole);

        rootModel->appendRow(row);
    }

    rootModel->setHorizontalHeaderLabels(QList<QString>() << tr("Quantity") << tr("Ingredient"));
    this->ui->rec_ing->setModel(rootModel);
    updateRecipePrice();
}

void MainWindow::showAddProductPopup()
{
    RecipeDialog* popUp = new RecipeDialog(currentRecipe, -1, this);
    popUp->setModal(true);
    popUp->show();
}

void MainWindow::showEditProductPopup()
{
    QModelIndexList indexes = this->ui->rec_ing->selectionModel()->selectedIndexes();
    if (indexes.size())
    {
        QModelIndex tableIndex = indexes.first();
        QVariant prodid = this->ui->rec_ing->model()->itemData(tableIndex)[SK_IdRole];
        RecipeDialog* popUp = new RecipeDialog(currentRecipe, prodid.toInt(), this);
        popUp->setModal(true);
        popUp->show();
    }
}


void MainWindow::removeProduct()
{
    QModelIndexList indexes = this->ui->rec_ing->selectionModel()->selectedIndexes();
    bool deleted = false;

    for (int i = 0; i < indexes.size(); i++)
    {
        QModelIndex tableIndex = indexes.at(i);
        QVariant prodId = this->ui->rec_ing->model()->itemData(tableIndex)[SK_IdRole];
        QSqlQuery query;
        query.prepare("DELETE FROM recipe_product WHERE recipe = :recId AND product = :prodId");
        query.bindValue(":prodId", prodId);
        query.bindValue(":recId", this->currentRecipe);
        query.exec();
        if (query.lastError().type() == QSqlError::NoError)
        {
            deleted = true;
        }
    }
    if (deleted)
    {
        updateIngredientsList();
        sta_loadTables();
    }
}
