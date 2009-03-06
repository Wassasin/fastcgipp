#include <fstream>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <asql/mysql.hpp>
#include <fastcgi++/manager.hpp>
#include <fastcgi++/request.hpp>
#include <fastcgi++/asqlbind.hpp>


void error_log(const char* msg)
{
	using namespace std;
	using namespace boost;
	static ofstream error;
	if(!error.is_open())
	{
		error.open("/tmp/errlog", ios_base::out | ios_base::app);
		error.imbue(locale(error.getloc(), new posix_time::time_facet()));
	}

	error << '[' << posix_time::second_clock::local_time() << "] " << msg << endl;
}

struct Log: public ASql::Data::Set
{
public:
	size_t numberOfSqlElements() const { return 4; }

	ASql::Data::Index getSqlIndex(const size_t index) const
	{
		switch(index)
		{
			case 0:
				return ipAddress.getInt();
			case 1:
				return timestamp;
			case 2:
				return sessionId;
			case 3:
				return referral;
			default:
				return ASql::Data::Index();
		}
	}

	Fastcgipp::Http::Address ipAddress;
	ASql::Data::Datetime timestamp;
	Fastcgipp::Http::SessionId sessionId;
	ASql::Data::WtextN referral;
};


class Database: public Fastcgipp::Request<wchar_t>
{
	static const char insertStatementString[];
	static const char selectStatementString[];

	static ASql::MySQL::Connection sqlConnection;
	static ASql::MySQL::Statement insertStatement;
	static ASql::MySQL::Statement selectStatement;

	enum Status { START, FETCH } status;

	typedef ASql::Data::SetContainer<std::vector<Log> > LogContainer;
	
	boost::shared_ptr<LogContainer> selectSet;
	boost::shared_ptr<long long unsigned> rows;

	bool response();
public:
	Database(): status(START), selectSet(new LogContainer), rows(new long long unsigned(0)) {}
	static void initSql();
};

const char Database::insertStatementString[] = "INSERT INTO logs (ipAddress, timeStamp, sessionId, referral) VALUE(?, ?, ?, ?)";
const char Database::selectStatementString[] = "SELECT SQL_CALC_FOUND_ROWS ipAddress, timeStamp, sessionId, referral FROM logs ORDER BY timeStamp DESC LIMIT 10";

ASql::MySQL::Connection Database::sqlConnection(1);
ASql::MySQL::Statement Database::insertStatement(Database::sqlConnection);
ASql::MySQL::Statement Database::selectStatement(Database::sqlConnection);

void Database::initSql()
{
	sqlConnection.connect("localhost", "fcgi", "databaseExample", "fastcgipp", 0, 0, 0, "utf8");
	insertStatement.init(insertStatementString, sizeof(insertStatementString), &Log(), 0);
	selectStatement.init(selectStatementString, sizeof(selectStatementString), 0, &Log());
	sqlConnection.start();
}

bool Database::response()
{
	switch(status)
	{
		case START:
		{
			selectStatement.queue(EMPTY_SQL_SET, selectSet, EMPTY_SQL_INT, rows, Fastcgipp::asqlbind(callback));
			status=FETCH;
			return false;
		}
		case FETCH:
		{
			out << "Content-Type: text/html; charset=utf-8\r\n\r\n\
<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n\
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>\n\
	<head>\n\
		<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />\n\
		<title>fastcgi++: SQL Database example</title>\n\
	</head>\n\
	<body>\n\
		<h2>Showing " << selectSet->size() << " results out of " << *rows << "</h2>\n\
		<table>\n\
			<tr>\n\
				<td><b>IP Address</b></td>\n\
				<td><b>Timestamp</b></td>\n\
				<td><b>Session ID</b></td>\n\
				<td><b>Referral</b></td>\n\
			</tr>\n";

			for(LogContainer::iterator it(selectSet->begin()); it!=selectSet->end(); ++it)
			{
				out << "\
			<tr>\n\
				<td>" << it->ipAddress << "</td>\n\
				<td>" << it->timestamp << "</td>\n\
				<td>" << it->sessionId << "</td>\n\
				<td>" << it->referral << "</td>\n\
			</tr>\n";
			}

			out << "\
		</table>\n\
		<p><a href='database.fcgi'>Refer Me</a></p>\n\
	</body>\n\
</html>";
			
			boost::shared_ptr<Log> queryParameters(new Log);
			queryParameters->ipAddress = environment.remoteAddress;
			queryParameters->timestamp = boost::posix_time::second_clock::universal_time();
			if(environment.referer.size())
				queryParameters->referral = environment.referer;
			else
				queryParameters->referral.nullness = true;

			insertStatement.queue(queryParameters, EMPTY_SQL_CONT, EMPTY_SQL_INT, EMPTY_SQL_INT, Fastcgipp::asqlbind(callback));

			return true;
		}
	}
}

int main()
{
	try
	{
		Database::initSql();
		Fastcgipp::Manager<Database> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
