#include <gtest/gtest.h>

#include <QFile>
#include <QTemporaryDir>

#include "../include/globals.h"

#define private public
#include "../include/assembler/custom_pseudo_manager.h"
#undef private

class CustomPseudoManagerTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		original_custom_pseudo_path_ = globals::custom_pseudo_instructions_file_path;

		ASSERT_TRUE(temp_dir_.isValid());
		globals::custom_pseudo_instructions_file_path =
			std::filesystem::path(temp_dir_.path().toStdString()) / "custom_pseudo_instructions.json";

		QFile seedFile(QString::fromStdString(globals::custom_pseudo_instructions_file_path.string()));
		ASSERT_TRUE(seedFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
		ASSERT_EQ(seedFile.write("{}"), 2);
		seedFile.close();
	}

	void TearDown() override
	{
		globals::custom_pseudo_instructions_file_path = original_custom_pseudo_path_;
	}

	QTemporaryDir temp_dir_;
	std::filesystem::path original_custom_pseudo_path_;
};

TEST_F(CustomPseudoManagerTest, AddPseudoInstructionAndExpandSingleLine)
{
	QString error;
	const bool added = CustomPseudoManager::addCustomPseudoInstruction(
		"myadd r1 r2",
		"add x10 r1 r2",
		error);

	ASSERT_TRUE(added) << error.toStdString();

	const std::string source = "myadd x3 x4";
	const std::string expanded = CustomPseudoManager::expandPseudoInstruction(source);

	EXPECT_EQ(expanded, "add x10 x3 x4");
}

TEST_F(CustomPseudoManagerTest, ExpandPseudoInstructionWithMultipleExpansionLines)
{
	QString error;
	const bool added = CustomPseudoManager::addCustomPseudoInstruction(
		"save2 r1 r2",
		"addi x2 x2 -8\n"
		"sw r1 x2 0\n"
		"sw r2 x2 4",
		error);

	ASSERT_TRUE(added) << error.toStdString();

	const std::string source = "save2 x8 x9";
	const std::string expanded = CustomPseudoManager::expandPseudoInstruction(source);

	EXPECT_EQ(expanded,
			  "addi x2 x2 -8\n"
			  "sw x8 x2 0\n"
			  "sw x9 x2 4");
}

TEST_F(CustomPseudoManagerTest, KeepsSourceLineWhenArgumentCountDoesNotMatch)
{
	QString error;
	const bool added = CustomPseudoManager::addCustomPseudoInstruction(
		"mov2 r1 r2",
		"add r1 r2 x0",
		error);

	ASSERT_TRUE(added) << error.toStdString();

	const std::string source = "mov2 x1";
	const std::string expanded = CustomPseudoManager::expandPseudoInstruction(source);

	EXPECT_EQ(expanded, "mov2 x1");
}
