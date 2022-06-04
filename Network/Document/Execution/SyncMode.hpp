#pragma once
#include <QByteArray>
namespace Network
{
enum class ExpressionPolicy
{
  OnFirst,
  OnMajority,
  OnAll
};

enum class SyncMode
{
  NonCompensatedSync,
  CompensatedSync,
  NonCompensatedAsync,
  CompensatedAsync
};

enum class ShareMode
{
  Shared,
  Mixed,
  Free
};

struct MessagesAPI
{
  MessagesAPI();
  static const MessagesAPI& instance();

  const QByteArray command_new;
  const QByteArray command_undo;
  const QByteArray command_redo;
  const QByteArray command_index;
  const QByteArray lock;
  const QByteArray unlock;

  const QByteArray ping;
  const QByteArray pong;
  const QByteArray play;
  const QByteArray stop;

  const QByteArray session_portinfo;
  const QByteArray session_askNewId;
  const QByteArray session_idOffer;
  const QByteArray session_join;
  const QByteArray session_document;

  const QByteArray trigger_expression_true;
  const QByteArray trigger_previous_completed;
  const QByteArray trigger_entered;
  const QByteArray trigger_left;
  const QByteArray trigger_finished;
  const QByteArray trigger_triggered;

  const QByteArray trigger_triggered_compensated;

  const QByteArray interval_speed;

  const QByteArray netpit_in_message;
  const QByteArray netpit_out_message;
};
}
