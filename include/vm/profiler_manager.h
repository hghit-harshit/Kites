#pragma once

#include <QObject>
#include <QMap>
#include <QVariant>
#include <map>

class ProfilerManager : public QObject
{
	Q_OBJECT

public:
	explicit ProfilerManager(QObject* parent = nullptr);

	void Reset();
	void SetInstructionLineMapping(const std::map<unsigned int, unsigned int>& instructionToLineMapping);

signals:
	void lineExecutionCountsUpdated(const std::map<int, int>& lineExecutionCounts);
	void profilerReset();

public slots:
	void OnVMStateChanged(const QMap<QString, QVariant>& vmState);

private:
	int ResolveSourceLineFromState(const QMap<QString, QVariant>& vmState) const;

	std::map<unsigned int, unsigned int> instruction_to_line_mapping_;
	std::map<int, int> line_execution_counts_;
};